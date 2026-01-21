/*
  Drone Server Map View
  ------------------------------------------------------------
  This file initializes a Google Map and periodically polls /api/fox/geo
  to place or update markers for FoxNodes that report latitude/longitude.

  Note: Comments were added for clarity; program behavior is unchanged.
*/

// Map state:
// - map: Google Map instance
// - bounds: LatLngBounds used to auto-fit markers
// - markers: Map keyed by fox id -> {m: Marker, info: InfoWindow}
let map, bounds, markers = new Map();


// Called by the Google Maps JS API callback parameter.
function initMap(){
  map = new google.maps.Map(document.getElementById('map'), {
    center: {lat: 39.7392, lng: -104.9903},
    zoom: 7,
    mapTypeId: 'terrain'
  });
  bounds = new google.maps.LatLngBounds();
  refresh();
  setInterval(refresh, 5000);
}


// Poll the server for FoxNode geolocation rows and update markers.
async function refresh(){
  try{
    const res = await fetch('/api/fox/geo', {cache:'no-store'});
    if (!res.ok){
      console.error('geo fetch failed', await res.text());
      return;
    }
    const data = await res.json();
    updateMarkers(data);
  }catch(e){
    console.error(e);
  }
}


// Create, update, and remove markers based on the latest server response.
function updateMarkers(rows){
  const seen = new Set();
  for (const r of rows){
    if (typeof r.lat !== 'number' || typeof r.lon !== 'number') continue;
    const id = String(r.fox);
    seen.add(id);
    const pos = {lat: r.lat, lng: r.lon};
    if (!markers.has(id)){
      const m = new google.maps.Marker({position: pos, map, label: id});
      const info = new google.maps.InfoWindow();
      m.addListener('click', ()=>{
        info.setContent(infoHtml(r));
        info.open({anchor: m, map});
      });
      markers.set(id, {m, info});
    } else {
      const {m, info} = markers.get(id);
      m.setPosition(pos);
      if (info.getMap()){
        info.setContent(infoHtml(r));
      }
    }
    bounds.extend(pos);
  }
  for (const [id, obj] of markers){
    if (!seen.has(id)){
      obj.m.setMap(null);
      markers.delete(id);
    }
  }
  if (!bounds.isEmpty()) map.fitBounds(bounds, 40);
}


// Build the HTML for the marker info window (UTC timestamps).
function infoHtml(r){
  const lastSeen = r.last_seen_ts ? new Date(r.last_seen_ts*1000).toISOString().replace('T',' ').replace('Z',' UTC') : '—';
  const lastSample = r.last_ftsr_received ? new Date(r.last_ftsr_received*1000).toISOString().replace('T',' ').replace('Z',' UTC') : '—';
  const elev = (typeof r.elev_m === 'number') ? `${r.elev_m.toFixed(1)} m` : '—';
  return `
    <div style="min-width:220px">
      <div><strong>Fox ${r.fox}</strong></div>
      <div class="meta">IP: ${r.fip||''}</div>
      <div class="meta">Battery: ${r.battery_v ?? ''} V • RSSI: ${r.rx_dbm ?? ''} dBm</div>
      <div class="meta">Last seen: ${lastSeen}</div>
      <div class="meta">Last sample: ${lastSample}</div>
      <div class="meta">Lat,Lon: ${r.lat?.toFixed?.(6) ?? r.lat}, ${r.lon?.toFixed?.(6) ?? r.lon} • Elev: ${elev}</div>
      <div style="margin-top:6px"><a class="btn" href="/ui" target="_blank">Open Dashboard</a></div>
    </div>
  `;
}


// If the template did not inject a Maps API key, show a helpful message.
if (!window.MAPS_API_KEY){
  const el = document.getElementById('map');
  if (el) el.innerHTML = '<div style="padding:16px">Add an environment variable <code>GOOGLE_MAPS_API_KEY</code> and reload this page to see the map.</div>';
}