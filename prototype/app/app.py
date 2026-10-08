"""TabaPort hub: Flask + SQLite + serial reader. Runs on the dispatcher laptop, offline.

    SERIAL_URL=auto                      python app.py      # real ESP32 over USB
    SERIAL_URL=rfc2217://localhost:4000  python app.py      # Wokwi (development only)
    SERIAL_URL=fake                      python app.py      # no hardware, built-in simulator

Then open http://localhost:5000
"""
import os
import time

from flask import Flask, jsonify, redirect, render_template, request, send_from_directory, url_for

import db
import serial_reader
import state_engine
from gis import router

HERE = os.path.dirname(os.path.abspath(__file__))
app = Flask(__name__, static_folder="static", template_folder="templates")
app.config["JSON_SORT_KEYS"] = False


@app.after_request
def _cors(resp):
    """Flutter (and later phones on the same Wi-Fi) call this API from another origin."""
    resp.headers["Access-Control-Allow-Origin"] = "*"
    resp.headers["Access-Control-Allow-Headers"] = "Content-Type"
    resp.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
    return resp


@app.route("/api/<path:_any>", methods=["OPTIONS"])
def _cors_preflight(_any):
    return ("", 204)

_segments_geojson = None


def segments_geojson():
    global _segments_geojson
    if _segments_geojson is None:
        _segments_geojson = router.load_json("road_segments.geojson")
    return _segments_geojson


def seed_segments():
    """Every segment in the GeoJSON gets a row so the map can colour it (unknown until data arrives)."""
    for f in segments_geojson()["features"]:
        p = f["properties"]
        state_engine.ensure_segment(p["segment_id"], p.get("node_id"))


# ---------- pages ----------
@app.get("/")
def index():
    return redirect(url_for("map_page"))


@app.get("/map")
def map_page():
    return render_template("map.html", page="map")


@app.get("/jobs")
def jobs_page():
    return render_template("jobs.html", page="jobs")


@app.get("/node")
def node_page():
    return render_template("node.html", page="node", node_id=request.args.get("node", "drain_01"))


@app.get("/settings")
def settings_page():
    return render_template("settings.html", page="settings", cfg=db.all_cfg())


@app.get("/data/<path:name>")
def data_file(name):
    return send_from_directory(os.path.join(HERE, "data"), name)


# ---------- read API ----------
@app.get("/api/state")
def api_state():
    snap = state_engine.snapshot()
    jobs = db.query("SELECT status, COUNT(*) n FROM job GROUP BY status")
    snap["jobs"] = {r["status"]: r["n"] for r in jobs}
    snap["serial"] = dict(serial_reader.status)
    snap["now"] = time.time()
    snap["escalate_after_s"] = db.cfg_float("escalate_after_s", 180)
    snap["unknown_after_s"] = db.cfg_float("unknown_after_s", 10)
    return jsonify(snap)


@app.get("/api/readings")
def api_readings():
    node = request.args.get("node", "drain_01")
    minutes = int(request.args.get("minutes", 5))
    rows = db.query(
        "SELECT timestamp, water_level_cm, raw_ultrasonic_cm, rise_rate_cm_per_min, filtered_flag "
        "FROM node_reading WHERE node_id=? AND timestamp >= datetime('now', ?) ORDER BY timestamp",
        (node, f"-{minutes} minutes"))
    th = state_engine.thresholds(node)
    return jsonify({"node_id": node, "thresholds": th, "readings": rows})


@app.get("/api/jobs")
def api_jobs():
    show = request.args.get("show", "active")
    if show == "all":
        where = "1=1"
    elif show == "closed":
        where = "j.status IN ('closed','false_alarm')"
    else:
        where = "j.status IN ('open','acked','en_route','on_site')"
    rows = db.query(f"""
        SELECT j.*, s.state AS segment_state, s.node_id,
               (strftime('%s','now') - strftime('%s', j.created_at)) AS age_s,
               a.level_cm AS alert_level_cm, a.to_state AS alert_state
        FROM job j
        JOIN road_segment_state s ON s.segment_id = j.segment_id
        LEFT JOIN alert a ON a.alert_id = j.alert_id
        WHERE {where}
        ORDER BY CASE j.status WHEN 'open' THEN 0 WHEN 'acked' THEN 1 WHEN 'en_route' THEN 2 WHEN 'on_site' THEN 3 ELSE 9 END,
                 j.created_at DESC
        LIMIT 200""")
    names = {f["properties"]["segment_id"]: f["properties"]["name"] for f in segments_geojson()["features"]}
    for r in rows:
        r["segment_name"] = names.get(r["segment_id"], r["segment_id"])
    crews = [c.strip() for c in (db.cfg("crews") or "").split(",") if c.strip()]
    return jsonify({"jobs": rows, "crews": crews, "escalate_after_s": db.cfg_float("escalate_after_s", 180)})


@app.get("/api/alerts")
def api_alerts():
    return jsonify(db.query("SELECT * FROM alert ORDER BY created_at DESC LIMIT 100"))


@app.get("/api/reports")
def api_reports_list():
    return jsonify(db.query("SELECT * FROM human_report ORDER BY reported_at DESC LIMIT 100"))


@app.get("/api/config")
def api_config():
    return jsonify(db.all_cfg())


# ---------- job actions ----------
JOB_TRANSITIONS = {
    "ack": ("acked", "acked_at"),
    "en_route": ("en_route", None),
    "on_site": ("on_site", None),
    "close": ("closed", "closed_at"),
}


@app.post("/api/jobs/<int:job_id>/<action>")
def api_job_action(job_id, action):
    body = request.get_json(silent=True) or {}
    job = db.query_one("SELECT * FROM job WHERE job_id=?", (job_id,))
    if not job:
        return jsonify({"error": "no such job"}), 404
    if action == "assign":
        crew = (body.get("crew") or "").strip()
        if not crew:
            return jsonify({"error": "crew required"}), 400
        db.execute("UPDATE job SET assigned_crew=? WHERE job_id=?", (crew, job_id))
    elif action == "false_alarm":
        db.execute("UPDATE job SET status='false_alarm', closed_at=?, false_alarm_reason=? WHERE job_id=?",
                   (state_engine.now_iso(), body.get("reason") or "dispatcher judgement", job_id))
    elif action == "action":
        db.execute("UPDATE job SET action=? WHERE job_id=?", (body.get("action") or job["action"], job_id))
    elif action in JOB_TRANSITIONS:
        status, stamp = JOB_TRANSITIONS[action]
        if stamp:
            db.execute(f"UPDATE job SET status=?, {stamp}=COALESCE({stamp},?) WHERE job_id=?",
                       (status, state_engine.now_iso(), job_id))
        else:
            db.execute("UPDATE job SET status=? WHERE job_id=?", (status, job_id))
        if action == "ack" and job["status"] != "open":
            pass
    else:
        return jsonify({"error": "unknown action"}), 400
    # NB: closing a job never changes road_segment_state. The sensor does.
    return jsonify(db.query_one("SELECT * FROM job WHERE job_id=?", (job_id,)))


@app.post("/api/jobs")
def api_job_create():
    """Dispatcher opens a job by hand (e.g. from a phone call)."""
    body = request.get_json(silent=True) or {}
    seg = body.get("segment_id")
    if not seg:
        return jsonify({"error": "segment_id required"}), 400
    state_engine.ensure_segment(seg)
    db.execute("INSERT INTO job(segment_id, action, status) VALUES(?,?, 'open')",
               (seg, body.get("action") or "Send drainage crew to check inlet"))
    return jsonify(db.query_one("SELECT * FROM job ORDER BY job_id DESC LIMIT 1"))


# ---------- human reports (typed by dispatcher from a call/SMS) ----------
@app.post("/api/reports")
def api_report_create():
    body = request.get_json(silent=True) or {}
    text = (body.get("text") or "").strip()
    rtype = body.get("report_type") or "flooding"
    match = router.match_report(text) if text else None
    seg = body.get("segment_id") or (match["candidate_segments"][0] if match and match["candidate_segments"] else None)
    if seg:
        state_engine.ensure_segment(seg)
    db.execute("INSERT INTO human_report(segment_id, report_type, note, reporter_hash) VALUES(?,?,?,?)",
               (seg, rtype, text, body.get("reporter_hash")))
    # A human 'road_impassable' on a segment with no sensor is marked yellow (source human_report), never red automatically.
    if seg and rtype in ("flooding", "road_impassable", "blockage"):
        current = db.query_one("SELECT state, node_id FROM road_segment_state WHERE segment_id=?", (seg,))
        if current and current["node_id"] is None and current["state"] in ("unknown", "green"):
            state_engine.set_segment_state(seg, "yellow", "human_report", None, None, f"report: {text[:80]}")
    return jsonify({"segment_id": seg, "match": match})


# ---------- dispatcher override of a segment (e.g. after crew confirms clear) ----------
@app.post("/api/segments/<segment_id>/state")
def api_segment_state(segment_id):
    body = request.get_json(silent=True) or {}
    new = body.get("state")
    if new not in ("green", "yellow", "red", "unknown"):
        return jsonify({"error": "bad state"}), 400
    row = db.query_one("SELECT node_id FROM road_segment_state WHERE segment_id=?", (segment_id,))
    if row and row["node_id"]:
        return jsonify({"error": "segment has a sensor; its state follows the node"}), 409
    state_engine.set_segment_state(segment_id, new, "dispatcher", None, None, body.get("note"))
    return jsonify({"ok": True})


# ---------- routing (stretch) ----------
@app.post("/api/route")
def api_route():
    body = request.get_json(silent=True) or {}
    states = {r["segment_id"]: r["state"] for r in db.query("SELECT segment_id, state FROM road_segment_state")}
    res = router.route_between_segments(segments_geojson(), states, body.get("from"), body.get("to"))
    return jsonify(res), (400 if "error" in res else 200)


# ---------- config ----------
@app.post("/api/config")
def api_config_set():
    body = request.get_json(silent=True) or {}
    for k, v in body.items():
        db.set_cfg(k, v)
    # push new thresholds to the node so its LEDs agree with the map
    serial_reader.send(state_engine.cfg_line("drain_01"))
    return jsonify(db.all_cfg())


# ---------- dev helpers (only when SERIAL_URL=fake) ----------
@app.post("/api/dev/fake")
def api_dev_fake():
    if serial_reader.status.get("mode") != "fake":
        return jsonify({"error": "not in fake mode"}), 403
    body = request.get_json(silent=True) or {}
    for k in ("level_cm", "auto", "spike_every", "silent"):
        if k in body:
            serial_reader.fake[k] = body[k]
    return jsonify(serial_reader.fake)


@app.post("/api/dev/reset")
def api_dev_reset():
    """Clear readings, alerts, jobs, reports before a take. Keeps config and segment rows."""
    for t in ("node_reading", "alert", "job", "human_report"):
        db.execute(f"DELETE FROM {t}")
    db.execute("UPDATE road_segment_state SET state='unknown', source='sensor', notes=NULL, last_updated=?",
               (state_engine.now_iso(),))
    return jsonify({"ok": True})


_booted = False


def boot():
    """Start the database, watchdog, and sensor thread once per process.

    `python app.py` calls this, and so does Render's `uvicorn main:app`.
    """
    global _booted
    if _booted:
        return
    _booted = True
    db.connect()
    seed_segments()
    state_engine.start_watchdog()
    serial_reader.start()


def main():
    boot()
    # 0.0.0.0 is required on Render: the proxy does not connect to 127.0.0.1.
    host = os.environ.get("HOST", "0.0.0.0")
    port = int(os.environ.get("PORT", "5000"))
    print(f"TabaPort hub on http://{host}:{port}  (serial: {serial_reader.status['url']})")
    app.run(host=host, port=port, debug=False, threaded=True, use_reloader=False)


if __name__ == "__main__":
    main()
