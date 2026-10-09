// Shared: live /api/stream pushes each reading; a 2 s poll covers a dropped stream.
// Pages subscribe via TP.onState(fn). The map recolours from that same payload.
window.TP = (function () {
  const handlers = [];
  let last = null;

  async function getJSON(url, opts) {
    const r = await fetch(url, Object.assign({ headers: { 'Content-Type': 'application/json' } }, opts || {}));
    const j = await r.json().catch(() => ({}));
    if (!r.ok) throw new Error(j.error || r.statusText);
    return j;
  }
  const post = (url, body) => getJSON(url, { method: 'POST', body: JSON.stringify(body || {}) });

  function fmtAge(s) {
    s = Math.max(0, Math.round(s));
    if (s < 60) return s + ' s';
    const m = Math.floor(s / 60); const r = s % 60;
    return m + ' min ' + (r < 10 ? '0' : '') + r + ' s';
  }

  function toast(msg) {
    const t = document.createElement('div'); t.className = 'toast'; t.textContent = msg;
    document.body.appendChild(t); setTimeout(() => t.remove(), 2200);
  }

  function topbar(st) {
    const n = st.nodes['drain_01'];
    const badge = document.getElementById('st-node');
    const serial = document.getElementById('st-serial');
    serial.classList.toggle('on', !!(st.serial && st.serial.connected));
    serial.title = st.serial ? (st.serial.connected ? 'serial: ' + st.serial.url : 'serial: ' + (st.serial.last_error || 'disconnected')) : '';
    const seg = (st.segments || []).find(s => s.node_id === 'drain_01');
    const state = seg ? seg.state : 'unknown';
    badge.className = 'badge ' + state;
    if (n && !n.silent && n.level_cm !== undefined && state !== 'unknown') {
      badge.textContent = 'drain_01 · ' + state + ' · ' + n.level_cm.toFixed(1) + ' cm';
    } else if (n && n.fault) {
      badge.textContent = 'drain_01 · no echo';
    } else {
      badge.textContent = 'drain_01 · unknown' + (n && n.age_s ? ' · silent ' + fmtAge(n.age_s) : '');
    }
    const open = (st.jobs && st.jobs.open) || 0;
    const pill = document.getElementById('nav-open-count');
    pill.textContent = open; pill.classList.toggle('hidden', open === 0);
    document.getElementById('st-clock').textContent = new Date().toLocaleTimeString();
  }

  function apply(st) {
    last = st; topbar(st);
    handlers.forEach(h => { try { h(st); } catch (e) { console.error(e); } });
  }

  async function tick() {
    try { apply(await getJSON('/api/state')); }
    catch (e) { /* server down; keep trying */ }
  }
  setInterval(tick, 2000); tick();
  if (window.EventSource) {
    const es = new EventSource('/api/stream');
    es.onmessage = (ev) => { try { apply(JSON.parse(ev.data)); } catch (e) { console.error(e); } };
  }

  return { onState: fn => handlers.push(fn), getJSON, post, fmtAge, toast, last: () => last,
           COLORS: { green: '#2ecc71', yellow: '#f1c40f', red: '#e74c3c', unknown: '#7f8c8d' } };
})();
