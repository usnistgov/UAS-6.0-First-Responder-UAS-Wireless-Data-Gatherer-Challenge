# Data Ferry (Drone Server) (Flask + SQLite)
# ------------------------------------------------------------
# This file implements a lightweight HTTP server that:
# - Receives FoxNode updates (handshake, values, logs, bye)
# - Stores time-series readings in SQLite
# - Exposes a dashboard UI and JSON APIs for monitoring
#
# Note: Comments were added for clarity; program behavior is unchanged.
# Portions of this code was generated with assistance from ChatGPT (OpenAI) and Gemini (Google).
# AI Generated code has been reviewed and integrated by the project authors.
#
# Standard library: used to build and create the on-disk folder structure.
from pathlib import Path

# Base directory where templates/ and static/ files are expected to live.
base = Path("/home/pete/cmdsvr/prod/")
tpl = base / "templates"
st  = base / "static"
tpl.mkdir(parents=True, exist_ok=True)
st.mkdir(parents=True, exist_ok=True)


# Flask provides routing, request parsing, and template rendering.
from flask import Flask, request, jsonify, abort, render_template
import os, sqlite3, time


# Configuration (environment variables with defaults)
# - TEAM_NAME: displayed in UI and dump metadata
# - SERVER_FIXED_IP: returned to clients for reference
# - DB_PATH: SQLite file path
# - ADMIN_TOKEN: required for the /api/admin/clear endpoint
# - GOOGLE_MAPS_API_KEY: used by /map page for client-side Maps JS
TEAM_NAME        = os.environ.get("TEAM_NAME", "Flying Data Miners")
SERVER_FIXED_IP  = os.environ.get("SERVER_FIXED_IP", "192.168.40.20")
DB_PATH          = os.environ.get("DB_PATH", "drone_server.db")
ADMIN_TOKEN      = os.environ.get("ADMIN_TOKEN", "clearme")
GOOGLE_MAPS_API_KEY = os.environ.get("GOOGLE_MAPS_API_KEY", "AIzaSyBwV_B_B_B_B_B_B_B_B_B_B_B_B")


# SQLite schema. WAL improves concurrency for read-heavy dashboards.
# Tables:
# - foxnodes: one row per FoxNode (latest status and optional GPS)
# - readings: time-series sensor samples
# - logs: optional text logs sent by clients
# - dumps_meta: small metadata record used by dump endpoints
SCHEMA_SQL = """
PRAGMA journal_mode=WAL;
CREATE TABLE IF NOT EXISTS foxnodes (
  fox INTEGER PRIMARY KEY,
  fip TEXT,
  last_seen_ts INTEGER,
  last_ftsr_received INTEGER DEFAULT NULL,
  battery_v REAL DEFAULT NULL,
  rx_dbm INTEGER DEFAULT NULL,
  lat REAL DEFAULT NULL,
  lon REAL DEFAULT NULL,
  elev_m REAL DEFAULT NULL
);
CREATE TABLE IF NOT EXISTS readings (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  fox INTEGER NOT NULL,
  ts INTEGER NOT NULL,
  t INTEGER, c INTEGER, h INTEGER, l INTEGER, p INTEGER, x INTEGER, y INTEGER, z INTEGER
);
CREATE INDEX IF NOT EXISTS idx_readings_fox_ts ON readings(fox, ts);
CREATE TABLE IF NOT EXISTS logs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  fox INTEGER NOT NULL,
  ts INTEGER NOT NULL,
  ltxt TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS dumps_meta (
  id INTEGER PRIMARY KEY CHECK (id=1),
  first_dump_time INTEGER,
  last_dump_time INTEGER,
  earliest_takeoff INTEGER,
  last_landing INTEGER,
  sortie_count INTEGER DEFAULT 0
);
INSERT OR IGNORE INTO dumps_meta (id) VALUES (1);
"""


# Open a SQLite connection with Row objects for dict-like access.
def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


# Create tables/indexes if they do not already exist.
def init_db():
    with get_db() as db:
        db.executescript(SCHEMA_SQL)
init_db()


# Minimal schema migration: add geo columns if they are missing.
def migrate_schema():
    with get_db() as db:
        cols = {r[1] for r in db.execute('PRAGMA table_info(foxnodes)').fetchall()}
        if 'lat' not in cols:
            db.execute('ALTER TABLE foxnodes ADD COLUMN lat REAL')
        if 'lon' not in cols:
            db.execute('ALTER TABLE foxnodes ADD COLUMN lon REAL')
        if 'elev_m' not in cols:
            db.execute('ALTER TABLE foxnodes ADD COLUMN elev_m REAL')
migrate_schema()


# Flask application instance.
app = Flask(__name__)

# Helper: current Unix timestamp (seconds).
now_ts = lambda : int(time.time())


# Parse a JSON request body; respond 400 if missing.
def parse_json():
    data = request.get_json(force=True, silent=False)
    if data is None: abort(400, description="Expected JSON body")
    return data


# Insert or update a FoxNode row based on the client's 'core' object.
# Uses COALESCE to avoid overwriting known values with nulls.
def upsert_foxnode(db, core):
    fox = int(core.get("fox"))
    fip = core.get("fip")
    ftsr = int(core.get("ftsr", 0)) if core.get("ftsr") is not None else None
    fbv = core.get("fbv"); wrxs = core.get("wrxs")
    glat = core.get("glat"); glon = core.get("glon"); gele = core.get("gele")

    db.execute("""INSERT INTO foxnodes(fox, fip, last_seen_ts, last_ftsr_received, battery_v, rx_dbm, lat, lon, elev_m)
                  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                  ON CONFLICT(fox) DO UPDATE SET
                    fip=excluded.fip,
                    last_seen_ts=excluded.last_seen_ts,
                    last_ftsr_received=COALESCE(excluded.last_ftsr_received, foxnodes.last_ftsr_received),
                    battery_v=COALESCE(excluded.battery_v, foxnodes.battery_v),
                    rx_dbm=COALESCE(excluded.rx_dbm, foxnodes.rx_dbm),
                    lat=COALESCE(excluded.lat, foxnodes.lat),
                    lon=COALESCE(excluded.lon, foxnodes.lon),
                    elev_m=COALESCE(excluded.elev_m, foxnodes.elev_m)""",
                (fox, fip, now_ts(), ftsr, fbv, wrxs, glat, glon, gele))
    return fox


# Persist a batch of readings for a FoxNode.
# The 'data_obj' is expected to contain:
# - ftso: base timestamp
# - dump: list of samples with rts offsets and sensor fields
def store_readings(db, fox, data_obj):
    base_ts = int(data_obj.get("ftso", 0)); readings = data_obj.get("dump", [])
    rows = []
    rts = int(r["rts"])
    for r in readings:
        if "rts" not in r: continue
        def v(k):
            val = r.get(k)
            if isinstance(val, str): return None  # "F"
            return val
        ts = rts if rts > 1_000_000_000 else (base_ts + int(r["rts"]))
        rows.append((fox, ts, v("t"), v("c"), v("h"), v("l"), v("p"), v("x"), v("y"), v("z")))
    if rows:
        db.executemany("INSERT INTO readings(fox, ts, t, c, h, l, p, x, y, z) VALUES (?,?,?,?,?,?,?,?,?,?)", rows)
        db.execute("UPDATE foxnodes SET last_ftsr_received=? WHERE fox=?", (max(r[1] for r in rows), fox))


# Standardized server response wrapper returned to FoxNodes.
def server_reply(srep, stsb=None, stse=None):
    serv = {"srep": srep, "sip": SERVER_FIXED_IP, "sts": now_ts()}
    if stsb is not None: serv["stsb"] = int(stsb)
    if stse is not None: serv["stse"] = int(stse)
    return jsonify({"serv": serv})


# Client entrypoint (FoxNode POSTs). This forwards to /update.
@app.route("/", methods=["POST"])
def root_post(): return update()


# Main protocol handler.
# The client sends payloads with:
# - core.ccmd: one of hi, rval, rlog, bye
# Depending on ccmd, the server may request values (sval) or acknowledge (buby/done).
@app.route("/update", methods=["POST"])
def update():
    payload = parse_json(); core = payload.get("core")
    if not isinstance(core, dict): abort(400, description="Missing 'core' object")
    ccmd = core.get("ccmd")
    if ccmd not in ("hi","rval","rlog","bye"): abort(400, description=f"Invalid 'ccmd': {ccmd}")
    with get_db() as db:
        fox = upsert_foxnode(db, core)
        if ccmd == "hi":
            row = db.execute("SELECT last_ftsr_received FROM foxnodes WHERE fox=?", (fox,)).fetchone()
            last = row["last_ftsr_received"] if row else None
            ftso = int(core.get("ftso",0)) if core.get("ftso") is not None else None
            ftsr = int(core.get("ftsr",0)) if core.get("ftsr") is not None else None
            if last is not None and ftsr is not None and ftsr >= last+1:
                return server_reply("sval", stsb=last+1, stse=ftsr)
            elif ftso is not None and ftsr is not None and ftsr >= ftso:
                return server_reply("sval", stsb=ftso, stse=ftsr)
            else:
                return server_reply("buby")
        if ccmd == "rval":
            data = payload.get("data")
            if not isinstance(data, dict): abort(400, description="Missing 'data' object for 'rval'")
            store_readings(db, fox, data); return server_reply("buby")
        if ccmd == "rlog":
            ldat = payload.get("ldat", {}); ltxt = ldat.get("ltxt","")
            if not ltxt: abort(400, description="Missing 'ldat.ltxt' for 'rlog'")
            db.execute("INSERT INTO logs(fox, ts, ltxt) VALUES (?, ?, ?)", (fox, now_ts(), ltxt)); return server_reply("buby")
        if ccmd == "bye": return server_reply("done")


# Build the 'dump' JSON object in the expected challenge format.
# Optional time filtering is used by incremental dumps.
def build_dump_object(time_from=None, time_to=None):
    with get_db() as db:
        fox_rows = db.execute("SELECT fox FROM foxnodes ORDER BY fox").fetchall(); found = len(fox_rows)
        where, params = [], []
        if time_from is not None: where.append("ts >= ?"); params.append(int(time_from))
        if time_to is not None: where.append("ts <= ?"); params.append(int(time_to))
        ws = ("WHERE " + " AND ".join(where)) if where else ""
        total = db.execute(f"SELECT COUNT(*) AS cnt FROM readings {ws}", params).fetchone()["cnt"]
        rmin = db.execute(f"SELECT MIN(ts) AS m FROM readings {ws}", params).fetchone()
        rmax = db.execute(f"SELECT MAX(ts) AS m FROM readings {ws}", params).fetchone()
        takeoff = rmin["m"] if rmin and rmin["m"] is not None else None
        landing = rmax["m"] if rmax and rmax["m"] is not None else None
        meta = db.execute("SELECT * FROM dumps_meta WHERE id=1").fetchone()
        first_dump, last_dump, sorties = meta["first_dump_time"], meta["last_dump_time"], meta["sortie_count"]
        collectData = []
        for fr in fox_rows:
            fox = fr["fox"]
            rs = db.execute(f"SELECT ts,t,c,h,l,p,x,y,z FROM readings WHERE fox=? {('AND ' + ' AND '.join(where)) if where else ''} ORDER BY ts", [fox]+params).fetchall()
            if not rs: continue
            base = rs[0]["ts"]
            dump = []
            for r in rs:
                dump.append({"rts": int(r["ts"]-base),
                             "t": r["t"] if r["t"] is not None else "F",
                             "c": r["c"] if r["c"] is not None else "F",
                             "h": r["h"] if r["h"] is not None else "F",
                             "l": r["l"] if r["l"] is not None else "F",
                             "p": r["p"] if r["p"] is not None else "F",
                             "x": r["x"] if r["x"] is not None else "F",
                             "y": r["y"] if r["y"] is not None else "F",
                             "z": r["z"] if r["z"] is not None else "F"})
            collectData.append({"foxNodeId": fox, "data": {"fox": fox, "dlen": len(dump), "ftso": int(base), "dump": dump}})
        return {"dump": {"team": TEAM_NAME, "foundCount": found, "sortieCount": sorties,
                         "earliestTakeOff": takeoff, "lastLanding": landing,
                         "firstDumpTime": first_dump, "lastDumpTime": last_dump,
                         "totalSampleCount": total, "collectData": collectData}}


# Export all stored readings in a single dump payload.
# Also increments a simple sortie counter and tracks dump times.
@app.route("/dump/full")
def dump_full():
    with get_db() as db:
        ts = now_ts()
        row = db.execute("SELECT first_dump_time FROM dumps_meta WHERE id=1").fetchone()
        if row and row["first_dump_time"] is None:
            db.execute("UPDATE dumps_meta SET first_dump_time=?, last_dump_time=?, sortie_count=sortie_count+1 WHERE id=1", (ts, ts))
        else:
            db.execute("UPDATE dumps_meta SET last_dump_time=?, sortie_count=sortie_count+1 WHERE id=1", (ts,))
    return jsonify(build_dump_object())


# Export readings newer than a provided 'since' unix timestamp.
@app.route("/dump/incremental")
def dump_incremental():
    try: since = int(request.args.get("since","0"))
    except ValueError: abort(400, description="since must be an integer unix timestamp")
    return jsonify(build_dump_object(time_from=since+1))


# Render the HTML dashboard (templates/ui.html).
@app.route("/ui")
def ui(): return render_template("ui.html", team=TEAM_NAME, server_ip=SERVER_FIXED_IP)


# Dashboard summary endpoint used by the UI polling loop.
@app.route("/api/summary")
def api_summary():
    with get_db() as db:
        fox_count = db.execute("SELECT COUNT(*) AS c FROM foxnodes").fetchone()["c"]
        reading_count = db.execute("SELECT COUNT(*) AS c FROM readings").fetchone()["c"]
        last_ts_row = db.execute("SELECT MAX(ts) AS m FROM readings").fetchone()
        last_ts = last_ts_row["m"] if last_ts_row and last_ts_row["m"] is not None else None
        since = (last_ts - 300) if last_ts else 0
        rows = db.execute("SELECT fox, COUNT(*) as cnt, MIN(ts) as first_ts, MAX(ts) as last_ts FROM readings WHERE ts >= ? GROUP BY fox ORDER BY fox", (since,)).fetchall()
        latest_rows = db.execute("""SELECT r.fox, r.ts, r.t, r.c, r.h, r.l, r.p, r.x, r.y, r.z
                                    FROM readings r JOIN (SELECT fox, MAX(ts) as mts FROM readings GROUP BY fox) m
                                    ON r.fox=m.fox AND r.ts=m.mts ORDER BY r.fox""").fetchall()
    return jsonify({"team":TEAM_NAME,"server_ip":SERVER_FIXED_IP,"fox_count":fox_count,"reading_count":reading_count,"last_ts":last_ts,
                    "per_fox_recent":[dict(r) for r in rows], "latest":[dict(r) for r in latest_rows]})


# Return current FoxNode table rows for dashboard listing.
@app.route("/api/foxnodes")
def api_foxnodes():
    with get_db() as db:
        rows = db.execute("SELECT fox, fip, last_seen_ts, last_ftsr_received, battery_v, rx_dbm, lat, lon, elev_m FROM foxnodes ORDER BY fox").fetchall()
    return jsonify([dict(r) for r in rows])


# Return time-series readings for a given FoxNode.
# Supports optional 'since' and bounded 'limit' for UI performance.
@app.route("/api/fox/<int:fox>/series")
def api_fox_series(fox):
    try:
        since = request.args.get("since"); since = int(since) if since is not None else None
    except ValueError: abort(400, description="'since' must be integer")
    try:
        limit = int(request.args.get("limit","500")); limit = max(1, min(limit, 5000))
    except ValueError: abort(400, description="'limit' must be integer")
    with get_db() as db:
        if since is not None:
            rows = db.execute("SELECT ts,t,c,h,l,p,x,y,z FROM readings WHERE fox=? AND ts >= ? ORDER BY ts LIMIT ?", (fox, since, limit)).fetchall()
        else:
            rows = db.execute("SELECT ts,t,c,h,l,p,x,y,z FROM readings WHERE fox=? ORDER BY ts DESC LIMIT ?", (fox, limit)).fetchall()
            rows = list(reversed(rows))
    return jsonify([dict(r) for r in rows])


# Return only FoxNodes that currently have latitude and longitude.
# Used by /map to place markers.
@app.route("/api/fox/geo", methods=["GET"])
def api_fox_geo():
    with get_db() as db:
        rows = db.execute("""SELECT fox, fip, last_seen_ts, last_ftsr_received, battery_v, rx_dbm, lat, lon, elev_m
                            FROM foxnodes
                            WHERE lat IS NOT NULL AND lon IS NOT NULL
                            ORDER BY fox""").fetchall()
    return jsonify([dict(r) for r in rows])


# Admin endpoint to clear stored data (requires X-Admin-Token).
@app.route("/api/admin/clear", methods=["POST"])
def admin_clear():
    token = request.headers.get("X-Admin-Token", "")
    if not ADMIN_TOKEN or token != ADMIN_TOKEN:
        abort(403, description="Forbidden")
    with get_db() as db:
        db.execute("DELETE FROM readings")
        db.execute("DELETE FROM logs")
        db.execute("DELETE FROM foxnodes")  # wipe foxnodes too for clean discovery
        db.execute("""UPDATE dumps_meta SET first_dump_time=NULL, last_dump_time=NULL,
                      earliest_takeoff=NULL, last_landing=NULL, sortie_count=0 WHERE id=1""")
    return jsonify({"ok": True, "cleared": True})


# Lightweight health check for container/orchestration monitoring.
@app.route("/health")
def health():
    with get_db() as db:
        fx = db.execute("SELECT COUNT(*) AS c FROM foxnodes").fetchone()["c"]
        rd = db.execute("SELECT COUNT(*) AS c FROM readings").fetchone()["c"]
    return jsonify({"status":"ok","foxnodes":fx,"readings":rd,"server_ip":SERVER_FIXED_IP,"team":TEAM_NAME})


# Render the map page (templates/map.html) which loads static/map.js.
@app.route("/map", methods=["GET"])
def map_page():
    return render_template("map.html", team=TEAM_NAME, maps_api_key=GOOGLE_MAPS_API_KEY)


# Local development entrypoint.
# In production, a WSGI server can import 'app' directly.
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT","80")))
