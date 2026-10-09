"""State engine: reading -> segment state -> alert -> job. Plus the silence watchdog.

Rules (from the plan):
  * state from water level: yellow at yellow_cm, red at red_cm OR within 10 cm of the road edge
  * rise_rate over a 60 s window
  * a node silent for unknown_after_s makes its segment 'unknown' (never green)
  * transition into yellow/red writes an alert and opens a job if none is active
  * closing a job does not change the segment state; the sensor does
"""
import statistics
import threading
import time
from collections import deque
from datetime import datetime, timezone

import db

ACTIVE_JOB = ("open", "acked", "en_route", "on_site")
ROAD_MARGIN_CM = 10.0

# Joel's wording. Keyed by state; dispatcher can change it on the job.
DEFAULT_ACTION = {
    "yellow": "Send drainage crew to check inlet",
    "red": "Ask traffic police to close lane; send drainage crew to desilt inlet",
}

_last_seen = {}          # node_id -> monotonic seconds of last good reading
_last_reading = {}       # node_id -> dict (for /api/state)
_raw_window = {}         # node_id -> deque of last 5 raw cm (hub-side median, same rule as firmware)
SPIKE_CM = 10.0
_listeners = []          # callbacks(event_dict) e.g. send CFG to node
_lock = threading.Lock()
_cond = threading.Condition()
_rev = 0


def now_iso():
    return datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S")


def touch():
    """Wake map listeners. Called after a reading or a silence timeout."""
    global _rev
    with _cond:
        _rev += 1
        _cond.notify_all()


def wait_rev(seen, timeout=12):
    """Block until the revision changes, or until timeout. Returns the current revision."""
    with _cond:
        if _rev != seen:
            return _rev
        _cond.wait(timeout)
        return _rev


def segment_for(node_id):
    return db.cfg(f"node_segment.{node_id}", node_id)


def thresholds(node_id):
    return {
        "h_sensor_cm": db.cfg_float(f"h_sensor_cm.{node_id}", 50),
        "h_road_cm": db.cfg_float(f"h_road_cm.{node_id}", 60),
        "yellow_cm": db.cfg_float(f"yellow_cm.{node_id}", 8),
        "red_cm": db.cfg_float(f"red_cm.{node_id}", 15),
    }


def classify(level_cm, th):
    if level_cm is None:
        return "unknown"
    if level_cm >= th["red_cm"] or level_cm >= th["h_road_cm"] - ROAD_MARGIN_CM:
        return "red"
    if level_cm >= th["yellow_cm"]:
        return "yellow"
    return "green"


def rise_rate(node_id, level_cm):
    """cm per minute over the last 60 s. None if there is no reading that old."""
    row = db.query_one(
        "SELECT water_level_cm, timestamp FROM node_reading WHERE node_id=? "
        "AND timestamp <= datetime('now','-55 seconds') ORDER BY timestamp DESC LIMIT 1", (node_id,))
    if not row:
        return None
    t0 = datetime.strptime(row["timestamp"], "%Y-%m-%d %H:%M:%S").replace(tzinfo=timezone.utc)
    dt_min = (datetime.now(timezone.utc) - t0).total_seconds() / 60.0
    if dt_min <= 0:
        return None
    return round((level_cm - row["water_level_cm"]) / dt_min, 2)


def ensure_segment(segment_id, node_id=None):
    db.execute("INSERT OR IGNORE INTO road_segment_state(segment_id,node_id,state,source) VALUES(?,?,'unknown','sensor')",
               (segment_id, node_id))
    if node_id:
        db.execute("UPDATE road_segment_state SET node_id=? WHERE segment_id=? AND node_id IS NULL", (node_id, segment_id))


def set_segment_state(segment_id, new_state, source="sensor", node_id=None, level_cm=None, notes=None):
    """Update state; on transition into yellow/red create alert + job. Returns (old, new)."""
    ensure_segment(segment_id, node_id)
    row = db.query_one("SELECT state FROM road_segment_state WHERE segment_id=?", (segment_id,))
    old = row["state"] if row else "unknown"
    if old == new_state:
        return old, new_state
    with db.transaction() as c:
        c.execute("UPDATE road_segment_state SET state=?, last_updated=?, source=?, notes=COALESCE(?,notes) WHERE segment_id=?",
                  (new_state, now_iso(), source, notes, segment_id))
        if new_state in ("yellow", "red"):
            cur = c.execute("INSERT INTO alert(segment_id,node_id,from_state,to_state,level_cm) VALUES(?,?,?,?,?)",
                            (segment_id, node_id, old, new_state, level_cm))
            alert_id = cur.lastrowid
            active = c.execute(
                f"SELECT job_id FROM job WHERE segment_id=? AND status IN ({','.join('?'*len(ACTIVE_JOB))})",
                (segment_id, *ACTIVE_JOB)).fetchone()
            if active is None:
                c.execute("INSERT INTO job(segment_id,alert_id,action,status) VALUES(?,?,?,'open')",
                          (segment_id, alert_id, DEFAULT_ACTION[new_state]))
            elif new_state == "red":
                # escalate wording on the existing job, keep its status
                c.execute("UPDATE job SET action=?, alert_id=? WHERE job_id=?",
                          (DEFAULT_ACTION["red"], alert_id, active["job_id"]))
    return old, new_state


def ingest(msg):
    """msg: parsed JSON dict from the node. Returns the enriched reading or None."""
    node_id = msg.get("node_id", "drain_01")
    if msg.get("boot") or msg.get("cfg"):
        _fire({"type": "node_hello", "node_id": node_id})
        return None
    segment_id = segment_for(node_id)
    th = thresholds(node_id)

    if not msg.get("ok", True) or msg.get("raw_cm") is None:
        # node is alive but has no echo: not a reading, let the watchdog mark unknown
        with _lock:
            _last_reading[node_id] = {**(_last_reading.get(node_id) or {}), "fault": True, "miss": msg.get("miss")}
        touch()
        return None

    raw = float(msg["raw_cm"])
    # Hub-side median-of-5 (same rule as the firmware) so a floating bottle or a hand under the probe
    # cannot flip the map red. Hub also recomputes level from its own calibration, not the node's.
    win = _raw_window.setdefault(node_id, deque(maxlen=5))
    win.append(raw)
    med = statistics.median(win)
    filtered = bool(msg.get("filtered", False)) or (len(win) >= 3 and abs(raw - med) > SPIKE_CM)
    use_dist = med if filtered else raw
    level = round(max(0.0, th["h_sensor_cm"] - use_dist), 1)
    rate = rise_rate(node_id, level)
    db.execute("INSERT INTO node_reading(node_id,timestamp,water_level_cm,rise_rate_cm_per_min,filtered_flag,raw_ultrasonic_cm) "
               "VALUES(?,?,?,?,?,?)", (node_id, now_iso(), level, rate, int(filtered), raw))
    state = classify(level, th)
    old, new = set_segment_state(segment_id, state, "sensor", node_id, level)
    reading = {
        "node_id": node_id, "segment_id": segment_id, "ts": time.time(),
        "raw_cm": raw, "level_cm": level, "filtered": filtered, "rise_rate": rate,
        "state": state, "below_road_cm": round(th["h_road_cm"] - level, 1), "fault": False,
    }
    with _lock:
        _last_seen[node_id] = time.monotonic()
        _last_reading[node_id] = reading
    if old != new:
        _fire({"type": "transition", "segment_id": segment_id, "from": old, "to": new})
    touch()
    return reading


def watchdog_tick():
    timeout = db.cfg_float("unknown_after_s", 10)
    now = time.monotonic()
    with _lock:
        stale = [n for n, t in _last_seen.items() if now - t > timeout]
    for node_id in stale:
        set_segment_state(segment_for(node_id), "unknown", "sensor", node_id, None, "node silent")
        with _lock:
            if node_id in _last_reading:
                _last_reading[node_id] = {**_last_reading[node_id], "state": "unknown", "silent": True}
        touch()


def start_watchdog():
    def run():
        while True:
            try:
                watchdog_tick()
            except Exception as e:  # keep the thread alive
                print("[watchdog]", e)
            time.sleep(1)
    threading.Thread(target=run, daemon=True, name="watchdog").start()


def snapshot():
    with _lock:
        nodes = {k: dict(v) for k, v in _last_reading.items()}
    for n in nodes.values():
        if "ts" in n:
            n["age_s"] = round(time.time() - n["ts"], 1)
    segments = db.query("SELECT segment_id,node_id,state,last_updated,source,notes FROM road_segment_state")
    return {"nodes": nodes, "segments": segments}


def on_event(cb):
    _listeners.append(cb)


def _fire(ev):
    for cb in list(_listeners):
        try:
            cb(ev)
        except Exception as e:
            print("[event]", e)


def cfg_line(node_id):
    th = thresholds(node_id)
    return f"CFG h_sensor={th['h_sensor_cm']:.1f} yellow={th['yellow_cm']:.1f} red={th['red_cm']:.1f}\n"
