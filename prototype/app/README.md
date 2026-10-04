# prototype/app — TabaPort hub (dispatcher laptop)

Flask + SQLite + PySerial. Reads JSON lines from the ESP32 node over USB, keeps road segment state, opens jobs, serves three screens. Runs fully offline; Leaflet is vendored in `static/vendor`.

```
app.py              Flask routes, startup
db.py               SQLite helpers (WAL, thread safe)
schema.sql          node_reading, road_segment_state, alert, job, human_report, config
state_engine.py     reading -> state -> alert -> job; 10 s silence -> unknown
serial_reader.py    USB / rfc2217 / fake reader thread; sends CFG thresholds to the node
gis/router.py       gazetteer match, stage elevation, A* routing avoiding red (from Trevor's toolkit)
data/               blackspots.geojson (TP-01..17), road_segments.geojson, gazetteer.json, tabaport.db
templates/          map.html, jobs.html, node.html, settings.html
static/             app.css, app.js, vendor/leaflet
```

## Run

```bash
cd prototype/app
python3 -m venv ../../.venv && ../../.venv/bin/pip install -r requirements.txt   # once
../../.venv/bin/python app.py                       # real ESP32 on USB (auto-detect)
SERIAL_URL=/dev/ttyUSB0 ../../.venv/bin/python app.py   # pick a port by hand (COM5 on Windows)
SERIAL_URL=rfc2217://localhost:4000 ../../.venv/bin/python app.py   # Wokwi, development only
SERIAL_URL=fake ../../.venv/bin/python app.py       # no hardware; Dev controls appear on the Live node page
```

Open http://localhost:5000. Wi-Fi off during filming; nothing here needs it.

## Screens

- **Map** `/map`: road segments coloured green / yellow / red / unknown (dashed), 17 blackspots from the register, the sensor node, field report box (gazetteer match → segment), Route crew (A*, red removed, unknown ×2, yellow ×4).
- **Jobs** `/jobs`: unacknowledged first; Assign crew, Acknowledge, Crew en route, Crew on site, Close, False alarm. Open jobs flash after 3 min. Closing never changes the road state.
- **Live node** `/node`: depth now, cm below road edge, rise rate, raw vs filtered chart (last 5 min), link status.
- **Settings** `/settings`: H_sensor, H_road, yellow/red thresholds, crews. "Compute and save" applies Joel's 50 % / 80 % rule and pushes a `CFG` line to the node.

## Serial protocol

Node → hub, one JSON object per line at 115200:

```
{"node_id":"drain_01","raw_cm":41.2,"level_cm":8.8,"filtered":false,"state":"green","ok":true}
{"node_id":"drain_01","raw_cm":null,"level_cm":null,"filtered":false,"state":"unknown","ok":false,"miss":5}
```

Hub → node, on connect and whenever Settings are saved:

```
CFG h_sensor=50.0 yellow=8.0 red=15.0
```

The hub recomputes depth from `raw_cm` with its own calibration and its own median-of-5, so the laptop never depends on the node's thresholds being right.

## State rules

- yellow at `yellow_cm`, red at `red_cm` or within 10 cm of `h_road_cm`
- no reading for `unknown_after_s` (10 s) → segment `unknown`; never green without data
- transition into yellow/red → row in `alert`; opens a `job` if the segment has no active one
- a human report on a segment without a sensor marks it yellow (source `human_report`), never red
- dispatcher can override segments that have no sensor; sensor segments follow the node

## Before a take

```bash
curl -X POST localhost:5000/api/dev/reset    # clears readings, alerts, jobs, reports; keeps config
```

## API (for scripts and tests)

`GET /api/state` · `GET /api/readings?node=drain_01&minutes=5` · `GET /api/jobs?show=active|closed|all` · `GET /api/alerts` · `GET /api/reports` · `GET /api/config`
`POST /api/jobs` `{segment_id, action}` · `POST /api/jobs/<id>/assign` `{crew}` · `/ack` · `/en_route` · `/on_site` · `/close` · `/false_alarm` `{reason}` · `/action` `{action}`
`POST /api/reports` `{text, report_type}` · `POST /api/segments/<id>/state` `{state, note}` · `POST /api/route` `{from, to}` · `POST /api/config` `{key: value}`
`POST /api/dev/fake` `{level_cm, auto, spike_every, silent}` (fake mode only) · `POST /api/dev/reset`
