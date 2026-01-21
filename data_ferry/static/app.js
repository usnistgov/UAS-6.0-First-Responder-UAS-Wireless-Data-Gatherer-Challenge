/*
  Drone Server UI (dashboard)
  ------------------------------------------------------------
  This script:
  - Polls /api/summary and /api/foxnodes on an interval
  - Renders small sparkline canvases per FoxNode and sensor key
  - Supports a time window selector and optional moving-average smoothing
  - Provides a theme toggle stored in localStorage
  - Exposes an admin "clear data" action (X-Admin-Token) from the UI

  Note: Comments were added for clarity; program behavior is unchanged.
*/

// Immediately-invoked function expression (IIFE) to avoid leaking globals.
(function(){
  // Convenience selectors for single and multiple DOM elements.
  const $  = (sel)=>document.querySelector(sel);
  const $$ = (sel)=>document.querySelectorAll(sel);
  // Convert Unix epoch seconds to a readable UTC string.
  const fmtTs = (ts)=> ts ? new Date(ts*1000).toISOString().replace('T',' ').replace('Z',' UTC') : '—';

  // =============== THEME TOGGLE ===============
  const root = document.documentElement;
  const THEME_KEY = 'ds_theme';
  function applyTheme(mode){
    if (mode === 'light'){
      root.setAttribute('data-theme', 'light');
      $('#themeToggle') && ($('#themeToggle').checked = true);
    } else {
      root.setAttribute('data-theme', 'dark');
      $('#themeToggle') && ($('#themeToggle').checked = false);
    }
    localStorage.setItem(THEME_KEY, mode);
  }
  (function initTheme(){
    const saved = localStorage.getItem(THEME_KEY) || 'dark';
    applyTheme(saved);
    $('#themeToggle') && $('#themeToggle').addEventListener('change', ()=> applyTheme($('#themeToggle').checked ? 'light' : 'dark'));
  })();

  // =============== ADMIN CLEAR ===============
  async function clearData(){
    const token = prompt('Admin token (X-Admin-Token):');
    if (!token) return;
    const r = await fetch('/api/admin/clear', {method:'POST', headers:{'X-Admin-Token': token}});
    if (!r.ok){
      alert('Failed to clear: ' + (await r.text()));
      return;
    }
    alert('Cleared. Reloading…');
    location.reload();
  }
  (function bindAdmin(){
    const b = document.getElementById('btnClear');
    if (b){ b.addEventListener('click', clearData); }
  })();

  // =============== RENDER HELPERS ===============
  function sanitizeSeries(series, key) {
    const lastAtTs = new Map();
    for (const r of series || []) {
      const ts = Number(r.ts);
      if (!Number.isFinite(ts)) continue;
      const raw = r[key];
      const v = (raw === null || raw === undefined) ? null : Number(raw);
      lastAtTs.set(ts, Number.isFinite(v) ? v : null);
    }
    const out = Array.from(lastAtTs.entries())
      .map(([ts, v]) => ({ ts, v }))
      .sort((a, b) => a.ts - b.ts);
    let prev = -Infinity;
    return out.filter(p => {
      if (p.ts > prev) { prev = p.ts; return true; }
      return false;
    });
  }

  function smoothSeries(points, win) {
    if (!win || win < 3 || (win % 2) === 0) return points.slice();
    const half = Math.floor(win/2);
    const out = points.map(p => ({...p}));
    for (let i=0; i<points.length; i++){
      if (points[i].v === null) { out[i].v = null; continue; }
      let sum = 0, n = 0;
      for (let j=i-half; j<=i+half; j++){
        if (j < 0 || j >= points.length) continue;
        const v = points[j].v;
        if (v === null || !Number.isFinite(v)) continue;
        sum += v; n++;
      }
      out[i].v = n ? (sum/n) : null;
    }
    return out;
  }

  function downsampleToPixels(points, xscale, W) {
    const kept = [];
    let lastX = -1;
    for (const p of points) {
      const x = Math.round(xscale(p.ts));
      if (x === lastX) {
        kept[kept.length - 1] = p;
      } else {
        kept.push(p);
        lastX = x;
      }
    }
    return kept;
  }

  function drawSpark(canvas, series, key, color, smoothWin){
    const ctx = canvas.getContext('2d');
    const W = canvas.width, H = canvas.height;

    ctx.save();
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, W, H);
    ctx.strokeStyle = '#d0d7de';
    ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(0, H - 0.5); ctx.lineTo(W, H - 0.5); ctx.stroke();
    ctx.restore();

    let pts = sanitizeSeries(series, key);
    if (smoothWin && smoothWin >= 3) pts = smoothSeries(pts, smoothWin);

    const valid = pts.filter(p => p.v !== null);
    if (valid.length < 2) {
      ctx.fillStyle = '#222'; ctx.fillText('no data', 8, H/2);
      return;
    }

    const xmin = valid[0].ts;
    const xmax = valid[valid.length - 1].ts;
    const vmin = Math.min(...valid.map(p => p.v));
    const vmax = Math.max(...valid.map(p => p.v));

    const xscale = (x) => (W - 10) * (x - xmin) / Math.max(1, (xmax - xmin)) + 5;
    const yscale = (v) => H - ((H - 10) * (v - vmin) / Math.max(1e-9,(vmax - vmin)) + 5);

    const pruned = downsampleToPixels(valid, xscale, W);

    const GAP_SEC = Math.max(10, Math.round((xmax - xmin) / 20));
    ctx.beginPath();
    let started = false;
    let prevTs = pruned[0].ts;

    for (const p of pruned) {
      const x = xscale(p.ts);
      const y = yscale(p.v);

      const bigGap = (p.ts - prevTs) > GAP_SEC;
      if (!started || bigGap) {
        ctx.moveTo(x, y);
        started = true;
      } else {
        ctx.lineTo(x, y);
      }
      prevTs = p.ts;
    }

    ctx.lineWidth = 2;
    ctx.strokeStyle = color || '#000000';
    ctx.stroke();
  }

  // =============== DATA FETCH ===============

  // Fetch JSON with cache disabled so the UI always shows current state.
  async function fetchJSON(url){
    const r = await fetch(url, {cache:'no-store'});
    if (!r.ok) throw new Error(await r.text());
    return r.json();
  }


  // Populate the top-level summary stats (counts and last sample time).
  async function loadSummary(){
    const s = await fetchJSON('/api/summary');
    $('#foxCount').textContent = s.fox_count;
    $('#readingCount').textContent = s.reading_count;
    $('#lastTs').textContent = fmtTs(s.last_ts);
  }

  function rowHtml(fox){
    const lastSeen = fmtTs(fox.last_seen_ts);
    const lastRcv = fox.last_ftsr_received ? fmtTs(fox.last_ftsr_received) : '—';
    return `<tr>
      <td>${fox.fox}</td>
      <td>${fox.fip||''}</td>
      <td>${lastSeen}</td>
      <td>${lastRcv}</td>
      <td>${fox.battery_v??''}</td>
      <td>${fox.rx_dbm??''}</td>
      <td><button data-fox="${fox.fox}" class="btnView">View</button></td>
    </tr>`;
  }


  // Populate the FoxNode table and bind click handlers for per-node charts.
  async function loadFoxnodes(){
    const nodes = await fetchJSON('/api/foxnodes');
    $('#foxRows').innerHTML = nodes.map(rowHtml).join('');
    $$('.btnView').forEach(b=> b.addEventListener('click', ()=> addChartsForFox(Number(b.dataset.fox)) ));
  }

  function panelTemplate(fox){
    return `<div class="panel" id="panel-${fox}">
      <h3>Fox ${fox}</h3>
      <div class="canvas-row">
        <div><label>T (°C)</label><canvas id="c-${fox}-t" width="380" height="120"></canvas></div>
        <div><label>H (%RH)</label><canvas id="c-${fox}-h" width="380" height="120"></canvas></div>
        <div><label>P (hPa)</label><canvas id="c-${fox}-p" width="380" height="120"></canvas></div>
      </div>
      <div class="canvas-row">
        <div><label>C</label><canvas id="c-${fox}-c" width="380" height="120"></canvas></div>
        <div><label>Lux</label><canvas id="c-${fox}-l" width="380" height="120"></canvas></div>
        <div><label>Accel X</label><canvas id="c-${fox}-x" width="380" height="120"></canvas></div>
      </div>
      <div class="canvas-row">
        <div><label>Accel Y</label><canvas id="c-${fox}-y" width="380" height="120"></canvas></div>
        <div><label>Accel Z</label><canvas id="c-${fox}-z" width="380" height="120"></canvas></div>
        <div></div>
      </div>
    </div>`;
  }

  // =============== SMOOTHING UI ===============
  const SMOOTH_KEY = 'ds_smooth';
  function getSmoothWin(){
    const v = Number(localStorage.getItem(SMOOTH_KEY) || 0);
    return Number.isFinite(v) ? v : 0;
  }
  function setSmoothWin(v){
    localStorage.setItem(SMOOTH_KEY, String(v));
  }
  function ensureSmoothingControl(){
    const anchor = $('#winSel');
    if (!anchor || $('#smoothSel')) return;
    const lbl = document.createElement('label'); lbl.textContent = 'Smoothing:'; lbl.setAttribute('for','smoothSel');
    lbl.style.marginLeft = '12px';
    const sel = document.createElement('select'); sel.id = 'smoothSel';
    sel.innerHTML = `
      <option value="0">Off</option>
      <option value="3">MA(3)</option>
      <option value="5">MA(5)</option>
      <option value="9">MA(9)</option>
    `;
    sel.value = String(getSmoothWin());
    anchor.parentElement.insertBefore(lbl, anchor.nextSibling);
    anchor.parentElement.insertBefore(sel, lbl.nextSibling);
    sel.addEventListener('change', ()=>{
      setSmoothWin(Number(sel.value));
      $$('.panel').forEach(async p => {
        const fox = Number(p.id.split('-')[1]);
        await refreshFoxCharts(fox);
      });
    });
  }

  async function addChartsForFox(fox){
    ensureSmoothingControl();
    const grid = $('#chartGrid');
    let panel = document.getElementById('panel-'+fox);
    if (!panel){
      panel = document.createElement('div');
      panel.innerHTML = panelTemplate(fox);
      grid.appendChild(panel);
    }
    await refreshFoxCharts(fox);
  }


  // Pull time-series data for one FoxNode and redraw all sparklines.
  async function refreshFoxCharts(fox){
    const seconds = Number($('#winSel').value || '0');
    const smoothWin = getSmoothWin();
    let url = `/api/fox/${fox}/series?limit=500`;
    if (seconds > 0){
      const now = Math.floor(Date.now()/1000);
      url += `&since=${now - seconds}`;
    }
    const series = await fetchJSON(url);
    drawSpark(document.getElementById(`c-${fox}-t`), series, 't', '#000000', smoothWin);
    drawSpark(document.getElementById(`c-${fox}-h`), series, 'h', '#1f6feb', smoothWin);
    drawSpark(document.getElementById(`c-${fox}-p`), series, 'p', '#d73a49', smoothWin);
    drawSpark(document.getElementById(`c-${fox}-c`), series, 'c', '#2da44e', smoothWin);
    drawSpark(document.getElementById(`c-${fox}-l`), series, 'l', '#bf8700', smoothWin);
    drawSpark(document.getElementById(`c-${fox}-x`), series, 'x', '#8250df', smoothWin);
    drawSpark(document.getElementById(`c-${fox}-y`), series, 'y', '#0ea5e9', smoothWin);
    drawSpark(document.getElementById(`c-${fox}-z`), series, 'z', '#8b949e', smoothWin);
  }

  // =============== MAIN LOOP ===============

  // Main polling loop: refresh summary, table, and any visible chart panels.
  async function tick(){
    try{
      await loadSummary();
      const had = document.querySelectorAll('.panel').length > 0;
      await loadFoxnodes();
      if (had){
        document.querySelectorAll('.panel').forEach(async p => {
          const fox = Number(p.id.split('-')[1]);
          await refreshFoxCharts(fox);
        });
      }
    }catch(e){ console.error(e); }
    finally{ setTimeout(tick, 5000); }
  }

  $('#winSel') && $('#winSel').addEventListener('change', ()=> {
    document.querySelectorAll('.panel').forEach(async p => {
      const fox = Number(p.id.split('-')[1]);
      await refreshFoxCharts(fox);
    });
  });

  tick();
})();
