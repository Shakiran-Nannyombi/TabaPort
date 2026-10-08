# Software Requirements Specification (SRS)
## TabaPort — Offline-First Flood Dispatch System for Kampala
### Generation 1 Prototype

---

**Document version:** 1.0  
**Date:** October 2026  
**Project:** TabaPort — IEEE Response Quest Challenge Phase 3, Concept #5267  
**Software Author:** Sharif  
**Team:** Shakiran Nannyombi (firmware), Sharif (software/hub), Eric (hardware build), Joel (thresholds and dispatcher language)  
**Deadline:** 9 October 2026

---

## Table of Contents

1. Introduction
2. Overall Description
3. System Architecture
4. Functional Requirements
5. Non-Functional Requirements
6. ESP32 to Hub Communication Specification
7. Hub REST API Specification
8. Database Schema
9. Constraints and Assumptions
10. Acceptance Criteria for the Prototype

---

## 1. Introduction

### 1.1 Purpose

This SRS defines the complete software and hardware communication requirements for the TabaPort Generation 1 prototype. It is the reference document the team follows when building, wiring, coding, and filming the IEEE Phase 3 submission.

### 1.2 Project Summary

When heavy rain hits Kampala, culverts at Clock Tower, Bwaise, and Kalerwe can overflow in minutes. Mobile networks often congest at the same moment. Field crews then cannot see which drain is rising, which road is still passable, or where to send the next truck.

TabaPort is an offline-first flood dispatch tool. It runs entirely on a laptop at the dispatcher desk — no cloud, no internet required.

**What it does in one sentence:**  
A water-level sensor in a culvert feeds a coloured road map (green / yellow / red) and a ranked job list on a laptop, so a dispatcher can send the right crew to the right drain before the road floods.

### 1.3 Scope of the Prototype

The prototype covers exactly one node connected to one laptop over USB. Everything beyond this is Generation 2.

| In scope (prototype) | Out of scope (Generation 2) |
|---|---|
| One ESP32 + JSN-SR04T over USB | LoRa, GSM, SIM800L |
| Laptop as the hub | Raspberry Pi field hub |
| Offline Leaflet map (GeoJSON) | Cloud tile providers |
| SQLite local database | Any cloud database |
| Flask web server on localhost | Production web server |
| Green / yellow / red segment states | Cell-broadcast SMS to public |
| Job lifecycle (open → closed) | KCCA system integration |
| Human report entry by dispatcher | Automated USSD gateway |
| Deliberate USB pull demo | Solar power / battery |

### 1.4 Definitions

| Term | Meaning |
|---|---|
| Node | The ESP32 + JSN-SR04T unit at the drain |
| Hub | The laptop running the Flask app |
| Segment | A named road section (e.g. `clock_tower_01`) |
| Level | Water depth in cm above the drain floor |
| H_sensor | Distance from the probe face to the drain floor (cm), measured dry |
| H_road | Distance from the road/path edge to the drain floor (cm) |
| State | One of: `green`, `yellow`, `red`, `unknown` |
| Alert | A record created when state transitions to yellow or red |
| Job | A work order created from an alert; assigned to a crew |
| CFG line | A plain-text command sent from the hub to the ESP32 to update thresholds |

---

## 2. Overall Description

### 2.1 System Context Diagram

```
┌──────────────────────────────────────────────────────┐
│  PHYSICAL WORLD                                      │
│                                                      │
│   Water rises in culvert                             │
│        │                                             │
│   JSN-SR04T probe (waterproof, faces down)           │
│        │  echo / trig pulses                         │
│   ESP32-WROOM-32 DevKit                              │
│        │  USB-C data cable (≤ 3 m, 115200 baud)      │
└────────┼─────────────────────────────────────────────┘
         │
         ▼  JSON text lines (upstream)
         │  CFG text lines (downstream)
┌────────────────────────────────────────────────────────┐
│  LAPTOP — offline, Wi-Fi off during demo               │
│                                                        │
│  serial_reader.py  (background thread)                 │
│       │ parsed dict                                    │
│  state_engine.py   (classify, alert, job)              │
│       │ SQL writes                                     │
│  db.py + SQLite    (tabaport.db, WAL mode)             │
│       │                                                │
│  app.py (Flask, http://localhost:5000)                 │
│       │ JSON over HTTP                                 │
│  Browser                                               │
│    ├── map.html    (Leaflet + GeoJSON road colours)    │
│    ├── jobs.html   (dispatch job list)                 │
│    ├── node.html   (live chart for the sensor)         │
│    └── settings.html (thresholds, crew names)          │
└────────────────────────────────────────────────────────┘
```

### 2.2 Actors

| Actor | Role |
|---|---|
| ESP32 node | Measures water level; sends JSON; receives CFG |
| Dispatcher | Uses the browser to read the map, manage jobs, enter reports |
| Field crew | Named in jobs; status updated by dispatcher from radio/phone |
| Boda rider / informant | Phones or radios in a report; dispatcher types it |

---

## 3. System Architecture

### 3.1 Component Summary

```
main.c          ESP-IDF firmware on the ESP32
serial_reader.py  reads the USB port, calls state_engine
state_engine.py   classifies readings, manages alerts and jobs
db.py             SQLite helpers (thread-safe, WAL mode)
schema.sql        table definitions and default config
app.py            Flask routes (pages + REST API)
gis/router.py     GeoJSON segment loader and route planner
static/app.js     browser polling and Leaflet map logic
static/app.css    map and jobs styling
templates/        Jinja2 HTML pages
data/             tabaport.db, road_segments.geojson, blackspots.geojson
```

### 3.2 Data Flow (one reading, end to end)

```
Step  Who             What
────  ──────────────  ────────────────────────────────────────────────
 1    ESP32 firmware  fires 20 µs TRIG pulse on GPIO5 every 500 ms
 2    JSN-SR04T       returns ECHO pulse; duration ∝ distance to water
 3    firmware        raw_cm = (echo_µs × 0.0343) / 2
 4    firmware        applies moving-median filter (window = 5 samples)
 5    firmware        level_cm = h_sensor_cm − filtered_cm
 6    firmware        sets LED colour (GPIO25/26/27) and buzzer (GPIO14)
 7    firmware        sends JSON line on UART0 (USB), 115200 baud
 8    serial_reader   reads bytes, buffers until \n, json.loads()
 9    state_engine    hub-side median filter (second pass, window = 5)
10    state_engine    recomputes level_cm from hub calibration values
11    state_engine    classifies: green / yellow / red
12    state_engine    writes row to node_reading (SQLite)
13    state_engine    if state changed: updates road_segment_state
14    state_engine    if yellow/red: writes alert row, opens job
15    Flask /api/state  browser polls every 2 s, reads from SQLite
16    browser         Leaflet recolours the road segment on the map
17    browser         Jobs table shows new open job
```

---

## 4. Functional Requirements

### FR-01 — Sensor Reading
The ESP32 shall measure the distance to the water surface using the JSN-SR04T at a rate of 2 Hz (every 500 ms).

### FR-02 — On-Board Filtering
The firmware shall apply a moving-median filter over the last 5 raw samples. If the current raw reading differs from the median by more than 10 cm (debris spike), the median shall be used instead and `"filtered": true` shall be set in the JSON output.

### FR-03 — Water Level Computation
The firmware and the hub shall each compute water level as:
```
level_cm = h_sensor_cm − filtered_distance_cm
level_cm = max(0, level_cm)
```
The hub's calibration values are the source of truth. The firmware uses defaults until the hub pushes a CFG line.

### FR-04 — Serial Output
The firmware shall output one JSON line per reading on UART0 at 115200 baud, terminated by `\n`. The format is:
```json
{"node_id":"drain_01","raw_cm":41.2,"level_cm":8.8,"filtered":false,"state":"green","ok":true}
```

### FR-05 — Fault Output
When no echo is received for `ECHO_TIMEOUT_US` (30 000 µs), the firmware shall output:
```json
{"node_id":"drain_01","raw_cm":null,"level_cm":null,"filtered":false,"state":"unknown","ok":false,"miss":3}
```
After 5 consecutive misses, all three LEDs shall blink together to indicate sensor fault.

### FR-06 — Boot Message
On power-on, after the LED self-test, the firmware shall output a boot message:
```json
{"node_id":"drain_01","boot":true,"h_sensor_cm":50.0,"yellow_cm":8.0,"red_cm":15.0}
```

### FR-07 — Config Reception
The firmware shall accept a CFG line from the hub at any time and update its thresholds in RAM:
```
CFG h_sensor=60.0 yellow=30.0 red=48.0\n
```
After applying the config, the firmware shall echo a confirmation message.

### FR-08 — LED State Indication
The LED colours on the node box shall always match the current state:

| State | Green LED | Yellow LED | Red LED |
|---|---|---|---|
| green | ON | OFF | OFF |
| yellow | OFF | ON | OFF |
| red | OFF | OFF | ON |
| unknown / fault | ALL blink | ALL blink | ALL blink |

### FR-09 — Buzzer Alert
The buzzer shall emit one beep (2000 Hz, 150 ms) on the transition from any state to `red`. It shall not repeat until the state leaves red and returns to red again.

### FR-10 — Serial Auto-Detection
`serial_reader.py` shall auto-detect the correct COM port by scanning for CP210x / CH340 / USB-UART devices. It shall retry every 2 s if no port is found or the port disconnects.

### FR-11 — Watchdog
A background thread shall check every 1 s whether each node has sent a valid reading within `unknown_after_s` (default 10 s). If not, the segment state shall be set to `unknown`. A silent node shall never be shown as green.

### FR-12 — State Classification
The hub's state engine shall classify the current level using the stored thresholds:

| State | Rule |
|---|---|
| green | level_cm < yellow_cm |
| yellow | level_cm ≥ yellow_cm |
| red | level_cm ≥ red_cm **OR** level_cm ≥ (h_road_cm − 10 cm) |
| unknown | no valid reading for > unknown_after_s seconds |

### FR-13 — Alert Creation
When a segment transitions into `yellow` or `red`, the state engine shall write a row to the `alert` table recording: segment_id, node_id, from_state, to_state, level_cm, and created_at.

### FR-14 — Job Creation
When a segment transitions into `yellow` or `red` and there is no active job for that segment, the state engine shall open a new job with status `open` and the default action text:

| Trigger state | Default action |
|---|---|
| yellow | "Send drainage crew to check inlet" |
| red | "Ask traffic police to close lane; send drainage crew to desilt inlet" |

If a job is already active and the state escalates to `red`, the existing job's action text shall be updated to the red wording.

### FR-15 — Job Lifecycle
The dispatcher shall be able to move a job through the following states:

```
open → (assign crew) → acked → en_route → on_site → closed
                                                    ↘ false_alarm (at any step)
```

Closing a job shall NOT change the segment state. Only the sensor changes the segment state.

### FR-16 — Config Push
When the dispatcher saves thresholds on the Settings page, the Flask app shall:
1. Write the new values to the `config` table in SQLite.
2. Immediately send a CFG line to the ESP32 over the USB serial port so the node LEDs stay in sync with the map.

### FR-17 — Offline Map
The map page shall render a Leaflet map using locally stored raster tiles and a GeoJSON file (`road_segments.geojson`). It shall function with no internet connection. Each road segment shall be coloured according to its current state in `road_segment_state`.

### FR-18 — Live Polling
The browser shall poll `GET /api/state` every 2 s and update the map colours, the status bar, and the job count without reloading the page.

### FR-19 — Human Report Entry
The dispatcher shall be able to type a field report (received by phone or radio) into the UI. For a segment with no sensor, a report of type `flooding`, `road_impassable`, or `blockage` shall set that segment to `yellow`. It shall never automatically set it to `red` without a sensor.

### FR-20 — Disconnection Demo
When the USB cable is pulled, the map shall turn the affected segment grey (`unknown`) within `unknown_after_s` seconds (default 10 s). When the cable is reconnected, the serial reader shall reconnect automatically and restore the live state from sensor data.

### FR-21 — Rise Rate Display
The node detail page (`/node`) shall show the rise rate in cm/min, computed over the last 60 s of readings. This is for display only in Generation 1; it does not change the alert state.

### FR-22 — Dev / Fake Mode
When `SERIAL_URL=fake`, the system shall run a built-in simulator that generates realistic readings (slow rise over ~3 minutes, then fall) without any physical hardware.

---

## 5. Non-Functional Requirements

### NFR-01 — Offline Operation
The entire system shall function with no internet connection. No API keys, no CDN, no cloud tile provider. All assets (tiles, GeoJSON, JS libraries) shall be served from the local filesystem.

### NFR-02 — Latency
A state change detected by the sensor shall appear on the browser map within 4 s:
- Sensor reads every 500 ms
- Hub processes within ~100 ms
- Browser polls every 2 s
- Total worst case: 0.5 + 0.1 + 2.0 + rendering ≤ 4 s

### NFR-03 — Reliability
A silent node shall be shown as `unknown` (grey), never as `green`. The watchdog enforces this within `unknown_after_s` (10 s default).

### NFR-04 — Data Accuracy
The hub shall apply its own median filter (window = 5) independently of the firmware filter. A floating object under the probe shall not flip the segment to `red` if the median of the last 5 readings is still below the red threshold.

### NFR-05 — Privacy
Phone numbers submitted with human reports shall be hashed (salted SHA-256) before storage. Raw phone numbers shall never be written to the database. The salt shall be stored on the laptop only.

### NFR-06 — Thread Safety
The SQLite database shall run in WAL mode. All writes from the serial reader thread and Flask request threads shall go through `db.py`'s `threading.RLock()`. No direct `sqlite3.connect()` calls outside `db.py`.

### NFR-07 — Portability
The hub software shall run on Python 3.10+ on Linux, macOS, and Windows. No native compiled dependencies beyond PySerial and Flask.

### NFR-08 — Cost
The total hardware cost for one working node shall be under UGX 200,000 (approximately US$55), using components available at Neriko Electronics, Wandegeya.

---

## 6. ESP32 to Hub Communication Specification

### 6.1 Physical Layer

| Parameter | Value |
|---|---|
| Interface | UART over USB (CP2102 chip on the DevKit) |
| Baud rate | 115 200 |
| Data bits | 8N1 |
| Cable | USB-C data cable, 2–3 m |
| Direction | Full duplex (readings up, config down) |
| Auto-reconnect | Hub retries every 2 s on disconnect |

### 6.2 Upstream Messages (ESP32 → Hub)

All messages are UTF-8 JSON objects, one per line, terminated by `\n`.

#### Normal reading
```json
{
  "node_id": "drain_01",
  "raw_cm": 41.2,
  "level_cm": 8.8,
  "filtered": false,
  "state": "green",
  "ok": true
}
```

#### Fault (no echo)
```json
{
  "node_id": "drain_01",
  "raw_cm": null,
  "level_cm": null,
  "filtered": false,
  "state": "unknown",
  "ok": false,
  "miss": 3
}
```

#### Boot confirmation
```json
{
  "node_id": "drain_01",
  "boot": true,
  "h_sensor_cm": 50.0,
  "yellow_cm": 8.0,
  "red_cm": 15.0
}
```

### 6.3 Downstream Messages (Hub → ESP32)

Plain text, one line, terminated by `\n`:

```
CFG h_sensor=60.0 yellow=30.0 red=48.0\n
```

The hub sends this line:
- On startup (after receiving the boot message)
- Whenever the dispatcher saves new thresholds in Settings

### 6.4 GPIO Pin Assignment

| Signal | Pin | Direction | Notes |
|---|---|---|---|
| Trigger | GPIO5 | Output | 20 µs pulse every 500 ms |
| Echo | GPIO18 | Input | Through 1 kΩ / 2 kΩ voltage divider — mandatory |
| Green LED | GPIO25 | Output | 220 Ω series resistor |
| Yellow LED | GPIO26 | Output | 220 Ω series resistor |
| Red LED | GPIO27 | Output | 220 Ω series resistor |
| Buzzer | GPIO14 | Output | LEDC PWM channel 0, 2000 Hz |
| Sensor power | VIN | Power | 5 V from USB — NOT 3.3 V |
| Ground | GND | Power | All grounds common |

**Do not use:** GPIO0, 2, 12, 15 (boot strapping pins), GPIO34–39 (input only).

### 6.5 Voltage Divider — Why It Is Mandatory

The JSN-SR04T Echo pin outputs 5 V. The ESP32 GPIO pins tolerate a maximum of 3.3 V. Without the divider, GPIO18 will be damaged silently.

```
  JSN-SR04T Echo (5 V)
        │
      [1 kΩ]
        │
        ├──────── GPIO18  (reads 3.3 V here)
        │
      [2 kΩ]
        │
       GND
```

Two 1 kΩ resistors in series from Echo to GND, with GPIO18 tapped between them, also works.

---

## 7. Hub REST API Specification

Base URL: `http://localhost:5000`

### Pages

| Route | Method | Description |
|---|---|---|
| `/` | GET | Redirects to `/map` |
| `/map` | GET | Leaflet map with road segment colours |
| `/jobs` | GET | Active job list |
| `/node` | GET | Live chart for one sensor node |
| `/settings` | GET | Threshold and config editor |

### Read Endpoints

#### GET /api/state
Browser polls this every 2 s. Returns the full snapshot.

**Response:**
```json
{
  "nodes": {
    "drain_01": {
      "node_id": "drain_01",
      "segment_id": "campus_drain_01",
      "raw_cm": 41.2,
      "level_cm": 8.8,
      "state": "green",
      "rise_rate": 0.3,
      "below_road_cm": 51.2,
      "age_s": 0.4,
      "fault": false
    }
  },
  "segments": [
    {
      "segment_id": "campus_drain_01",
      "node_id": "drain_01",
      "state": "green",
      "last_updated": "2026-10-07 09:14:22",
      "source": "sensor"
    }
  ],
  "jobs": { "open": 1, "acked": 0 },
  "serial": { "connected": true, "lines": 1204, "url": "COM5" },
  "escalate_after_s": 180,
  "unknown_after_s": 10
}
```

#### GET /api/readings?node=drain_01&minutes=5
Returns timestamped readings for the node chart.

#### GET /api/jobs?show=active
Returns the job list. `show` can be `active`, `closed`, or `all`.

**Response:**
```json
{
  "jobs": [
    {
      "job_id": 47,
      "segment_id": "campus_drain_01",
      "segment_name": "Campus drain",
      "action": "Send drainage crew to check inlet",
      "assigned_crew": null,
      "status": "open",
      "created_at": "2026-10-07 09:14:03",
      "age_s": 120,
      "alert_level_cm": 18.5,
      "alert_state": "yellow"
    }
  ],
  "crews": ["Team A - Central", "Team B - Kawempe", "Team C - Rubaga", "Traffic Police"],
  "escalate_after_s": 180
}
```

#### GET /api/config
Returns all key/value config entries.

### Write Endpoints

#### POST /api/jobs/{id}/{action}
Dispatcher actions on a job.

| action | Effect |
|---|---|
| `assign` | Sets `assigned_crew` (body: `{"crew": "Team A - Central"}`) |
| `ack` | Sets status to `acked`, records `acked_at` |
| `en_route` | Sets status to `en_route` |
| `on_site` | Sets status to `on_site` |
| `close` | Sets status to `closed`, records `closed_at` |
| `false_alarm` | Sets status to `false_alarm` (body: `{"reason": "debris under probe"}`) |

#### POST /api/jobs
Dispatcher opens a job manually (body: `{"segment_id": "campus_drain_01", "action": "..."}`).

#### POST /api/reports
Dispatcher enters a field report (body: `{"text": "...", "report_type": "flooding", "segment_id": "..."}`).

#### POST /api/config
Saves thresholds and immediately pushes CFG line to the ESP32.

```json
{
  "h_sensor_cm.drain_01": "60",
  "h_road_cm.drain_01": "75",
  "yellow_cm.drain_01": "30",
  "red_cm.drain_01": "48"
}
```

#### POST /api/dev/fake (only when SERIAL_URL=fake)
Controls the built-in simulator.

```json
{ "level_cm": 20.0, "auto": true, "spike_every": 10, "silent": false }
```

#### POST /api/dev/reset
Clears all readings, alerts, jobs, and reports before a fresh take. Keeps config.

---

## 8. Database Schema

Four tables plus a config key-value store. One SQLite file: `data/tabaport.db`, WAL mode.

### node_reading
Stores every valid sensor reading.

| Column | Type | Description |
|---|---|---|
| reading_id | INTEGER PK | Auto-increment |
| node_id | TEXT | Node identifier (`drain_01`) |
| timestamp | DATETIME | UTC wall clock |
| water_level_cm | REAL | Filtered water depth in cm |
| rise_rate_cm_per_min | REAL | Computed over last 60 s (nullable) |
| filtered_flag | BOOLEAN | 1 if hub median filter replaced the raw value |
| raw_ultrasonic_cm | REAL | Raw echo distance before filtering |

### road_segment_state
One row per road segment. State is updated by the sensor, human reports, or the dispatcher.

| Column | Type | Description |
|---|---|---|
| segment_id | TEXT PK | e.g. `campus_drain_01` |
| node_id | TEXT | Nullable — segment may have no sensor |
| state | TEXT | `green`, `yellow`, `red`, or `unknown` |
| last_updated | DATETIME | When state last changed |
| source | TEXT | `sensor`, `human_report`, or `dispatcher` |
| notes | TEXT | Optional free text |

### alert
Written on every yellow/red transition.

| Column | Type | Description |
|---|---|---|
| alert_id | INTEGER PK | Auto-increment |
| segment_id | TEXT FK | Which segment |
| node_id | TEXT | Which node |
| from_state | TEXT | Previous state |
| to_state | TEXT | New state (`yellow` or `red`) |
| level_cm | REAL | Water level at the moment of transition |
| created_at | DATETIME | When the alert fired |

### job
One work order per alert (or opened manually).

| Column | Type | Description |
|---|---|---|
| job_id | INTEGER PK | Auto-increment |
| segment_id | TEXT FK | Which segment |
| alert_id | INTEGER FK | Nullable (manual jobs have no alert) |
| action | TEXT | Joel's wording (dispatcher can edit) |
| assigned_crew | TEXT | Nullable until assigned |
| status | TEXT | `open`, `acked`, `en_route`, `on_site`, `closed`, `false_alarm` |
| created_at | DATETIME | When job was opened |
| acked_at | DATETIME | Nullable |
| closed_at | DATETIME | Nullable |
| false_alarm_reason | TEXT | Nullable |

### human_report
Field reports entered by the dispatcher.

| Column | Type | Description |
|---|---|---|
| report_id | INTEGER PK | Auto-increment |
| segment_id | TEXT FK | Nullable if location unknown |
| reported_at | DATETIME | When entered |
| report_type | TEXT | `blockage`, `flooding`, `road_impassable`, `clear` |
| note | TEXT | Free text (typed by dispatcher) |
| reporter_hash | TEXT | Salted SHA-256 of phone number (never raw) |

### config (key-value)

| Key | Default | Description |
|---|---|---|
| `node_segment.drain_01` | `campus_drain_01` | Maps node ID to segment ID |
| `h_sensor_cm.drain_01` | `50` | Probe face to drain floor (cm) — set on site |
| `h_road_cm.drain_01` | `60` | Road edge to drain floor (cm) — set on site |
| `yellow_cm.drain_01` | `8` | Yellow threshold (tank fallback; set from H_road on site) |
| `red_cm.drain_01` | `15` | Red threshold (tank fallback; set from H_road on site) |
| `unknown_after_s` | `10` | Seconds of silence before segment goes unknown |
| `escalate_after_s` | `180` | Seconds before an unacknowledged job flashes |
| `crews` | `Team A - Central,...` | Comma-separated crew names for the assign dropdown |

---

## 9. Constraints and Assumptions

### Hardware Constraints
- The JSN-SR04T has a blind zone of 25 cm. The probe face must be at least 30 cm above the highest water level that will be created during the demo.
- The probe face must point straight down (within 5° of vertical) and be at least 30 cm from any wall, otherwise the echo bounces off the wall and not the water.
- Only one program may hold the serial port at a time. Close `idf.py monitor` before starting `app.py`.

### Software Constraints
- Python 3.10 or higher is required on the hub laptop.
- Flask, PySerial, and their dependencies must be installed (`pip install -r requirements.txt`).
- The browser must be opened at `http://localhost:5000`, not as a local file (`file://`).
- Wi-Fi must be switched off during filming so the map's offline tile behaviour is clearly demonstrated.

### Calibration Constraints
- `h_sensor_cm` and `h_road_cm` must be measured on site with a tape measure before filming begins.
- The dry echo reading from the hub should be within 3 cm of `h_sensor_cm`. If the difference is greater, re-aim the probe.
- Thresholds must be entered in the Settings page and saved before filming, so the CFG line is pushed to the ESP32.

### Scope Constraints
- This prototype uses one node and one laptop. Multi-node, multi-hub, or cloud scenarios are not in scope.
- The routing feature (`/api/route`) is a stretch goal. It is not required for the Phase 3 video.

---

## 10. Acceptance Criteria for the Prototype

These are the pass/fail tests for the 9 October video. Every item must pass before filming.

### AC-01 — Power-on self test
When the ESP32 is powered on, the three LEDs shall flash in turn (green, yellow, red) and the buzzer shall beep once. The hub terminal shall show the boot JSON message. **Pass / Fail**

### AC-02 — Live reading on the hub
Within 2 s of connecting the USB cable, the node detail page (`/node`) shall show a moving chart with `raw_cm` values changing when a hand is waved under the probe. **Pass / Fail**

### AC-03 — Yellow transition
When water is poured into the tank until the level reaches the yellow threshold, the map segment shall turn yellow, a job shall appear on the jobs page with status `open`, and the yellow LED on the node box shall light. **Pass / Fail**

### AC-04 — Red transition and buzzer
When water is poured further until the level reaches the red threshold, the map segment shall turn red, the job action text shall escalate, the red LED shall light, and the buzzer shall beep once. **Pass / Fail**

### AC-05 — Job lifecycle
The dispatcher shall be able to: assign a crew, click Ack, click En Route, click On Site, and click Close on a job. The job status shall update on the screen after each action. **Pass / Fail**

### AC-06 — Sensor does not go green on job close
After clicking Close on the job, the segment state shall remain at whatever level the sensor currently reports. The segment shall only return to green if the sensor reading drops below the yellow threshold. **Pass / Fail**

### AC-07 — False alarm
The dispatcher shall be able to click False Alarm on an open job, enter a reason, and have the job disappear from the active list. **Pass / Fail**

### AC-08 — Disconnection
When the USB cable is pulled, the map segment shall turn grey (`unknown`) within 10 s. When the cable is plugged back in, the segment shall restore its live state within 4 s. **Pass / Fail**

### AC-09 — Config push
When the dispatcher changes a threshold on the Settings page and clicks Save, the new CFG line shall be sent to the ESP32 (confirmed by the hub log), and the LED colours shall reflect the updated thresholds on the next reading. **Pass / Fail**

### AC-10 — Offline map
With Wi-Fi switched off, the map shall still render tiles, show the road segments, and update colours in real time. **Pass / Fail**

### AC-11 — Human report
The dispatcher shall be able to type a field report on the reports page, select `flooding` as the type, assign it to a segment, and see that segment change from `unknown` to `yellow` on the map. **Pass / Fail**

### AC-12 — Rise rate display
The node detail page shall show the current rise rate in cm/min, updating every 2 s. **Pass / Fail**

---

*End of SRS — TabaPort Generation 1 Prototype*
