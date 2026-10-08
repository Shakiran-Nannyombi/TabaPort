# TabaPort mobile (Flutter)

Software requirements for the **phone dispatcher**. Sharif's Generation 1 SRS (`docs/TabaPort_SRS.docx`) still governs the laptop hub and the USB node. This README is that SRS, rewritten for the app we are building next: a Flutter client that talks to the same hub over Wi-Fi.

| Field | Value |
|---|---|
| Document | TabaPort mobile SRS / README |
| Version | 1.0 · October 2026 |
| Project | TabaPort — IEEE Response Quest Challenge, concept 5267 |
| Software | Sharif (hub API) + Shakiran (this app) |
| Package | `ug.tabaport.tabaport_mobile` |

## 1. How it is supposed to work

There are two links. They are not the same cable.

```
DRAIN
  JSN-SR04T  -->  ESP32
                      |  Generation 1 (today): USB-C serial, 115200
                      |  Generation 2 (later): Wi-Fi / MQTT from the field node
                      v
LAPTOP HUB  (Flask + SQLite)   prototype/app/app.py
  serial_reader.py  -->  state_engine.py  -->  tabaport.db
                      |
                      |  Wi-Fi on the same LAN  (HTTP REST, already specified in the SRS)
                      v
PHONE  (this Flutter app)
  Map · Jobs · Live node · Settings
```

**Today (Generation 1).** The ESP32 stays on USB to the laptop. The phone does **not** open `/dev/ttyUSB0`. It calls `http://<laptop-ip>:5000/api/...`. That is already how the browser works; the phone is the same API on a smaller screen.

**Later (field node).** The ESP32 joins Wi-Fi and publishes JSON (MQTT is the stand-in in `prototype/sysmatic/field_node`). The hub still owns state, jobs, and calibration. The phone still talks to the hub. We do not put serial, SQLite, or threshold maths on the phone.

So: USB is the sensor-to-hub link. Wi-Fi is the hub-to-phone link now, and later also the node-to-hub link. Yes, that is how it is supposed to work.

## 2. Scope

**In this app**

- Map of road segments coloured green / yellow / red / unknown (GeoJSON from the hub, no public tile server)
- Job list: assign, ack, en route, on site, close, false alarm
- Live node: depth, rise rate, serial status of the laptop
- Settings: hub address and the drain calibration (50 % / 80 %). The long hub config list stays on the laptop.
- Field report typed on the phone, gazetteer match on the hub

**Not in this app (stays on the laptop / node)**

- USB serial, PySerial, `idf.py monitor`
- Median filter, state engine, SQLite
- LoRa, GSM, solar, KCCA login, public SMS

## 3. Functional requirements (mobile)

| ID | Requirement |
|---|---|
| M-01 | App polls `GET /api/state` every 2 s. Same payload the browser uses. |
| M-02 | Map is Google Maps. Road segments from `GET /data/road_segments.geojson` are drawn on top and coloured from `road_segment_state`. |
| M-03 | Jobs from `GET /api/jobs`. Actions `POST /api/jobs/{id}/{assign,ack,en_route,on_site,close,false_alarm}`. Closing a job must not turn the segment green. |
| M-04 | Live node shows `level_cm`, `below_road_cm`, `rise_rate`, filter flag, and whether the **laptop** still has USB. |
| M-05 | The phone hub address is fixed to `https://tabaport.onrender.com`. |
| M-06 | Calibration POST `/api/config` with `h_sensor_cm.drain_01`, `h_road_cm.drain_01`, `yellow_cm.drain_01`, `red_cm.drain_01`. Hub pushes `CFG` over USB. |
| M-07 | Field report POST `/api/reports`. Hub never marks red from a human report alone. |
| M-08 | If the hub is unreachable, show an error. Do not invent green. |
| M-09 | Cleartext HTTP allowed (LAN). No API keys, no cloud. |

Hub REST, JSON serial format, GPIO map, schema, and acceptance tests AC-01…AC-12 remain exactly as in `docs/TabaPort_SRS.docx` sections 6–10.

## 4. Non-functional

- Dark UI, readable outdoors.
- Hub is the source of truth. Phone is a view + dispatcher controls.
- Phone numbers in reports are still hashed on the hub, never stored raw.
- Latency budget unchanged: state on screen within ~4 s of a real level change.

## 5. Run it

On the laptop (USB node or fake serial):

```bash
cd prototype/app
HOST=0.0.0.0 SERIAL_URL=fake ../../.venv/bin/python app.py
# real board: drop SERIAL_URL=fake  (auto-detect USB)
```

`HOST=0.0.0.0` is what lets a phone on the same Wi-Fi in. Leave it `127.0.0.1` only if you will never use the phone.

Find the laptop IP (`ip addr` or `hostname -I`), then:

```bash
export PATH="$HOME/flutter/bin:$PATH"
cd prototype/mobile
flutter pub get
flutter run -d linux          # this computer, hub URL http://127.0.0.1:5000
flutter run                   # phone uses https://tabaport.onrender.com
```

On the phone: **Settings** can change the hub address. It starts at `https://tabaport.onrender.com`. Linux desktop can stay on `http://127.0.0.1:5000`. Android emulator uses `http://10.0.2.2:5000`.

The phone needs internet to reach `https://tabaport.onrender.com` and to load Google Maps tiles.

## Google Maps key (once)

1. [Google Cloud Console](https://console.cloud.google.com/) → create a project → enable **Maps SDK for Android**.
2. Create an API key. Restrict it to Android apps, package name `ug.tabaport.tabaport_mobile`.
3. Add this line to `prototype/mobile/android/local.properties` (that file is not committed):

```
MAPS_API_KEY=your_key_here
```

Without that line the map is blank grey. The coloured drain lines still come from the hub.

## Put it on your phone

1. On the phone: Settings → About phone → tap **Build number** seven times. Then Settings → Developer options → **USB debugging** on.
2. Plug in the USB cable (a data cable, the same kind that flashes the ESP32). Accept the “allow USB debugging” prompt.
3. On the laptop:

```bash
export PATH="$HOME/flutter/bin:$PATH"
cd prototype/mobile
flutter devices          # your phone should be listed
flutter run              # installs and opens TabaPort; leave this terminal running
```

To install an APK you can open without the cable:

```bash
flutter build apk --debug
# file: build/app/outputs/flutter-apk/app-debug.apk
```

Copy `app-debug.apk` to the phone (USB file copy, or `adb install build/app/outputs/flutter-apk/app-debug.apk`) and open it. Android will ask you to allow installs from that source once.

Google Maps does not run on the Linux desktop build. Use the phone.

## 6. Project layout

```
lib/
  main.dart              shell, 2 s poll, four tabs
  hub_config.dart        hub address, Render by default on the phone
  api/hub_api.dart       REST client for the SRS section 7 API
  theme.dart             green / yellow / red / unknown
  screens/map_screen.dart
  screens/jobs_screen.dart
  screens/node_screen.dart
  screens/settings_screen.dart
```

## 7. Generation 1 hardware (unchanged)

ESP32 VIN→JSN-SR04T +5V, GPIO5 Trig, GPIO18 Echo through 1 kΩ / 2 kΩ, GPIO25/26/27 LEDs, GPIO14 buzzer. See `docs/phase3/wiring.md`. The phone never sees those pins.

## 8. Later: Wi-Fi on the node

When `field_node` is on a real board, the laptop hub gains a network reader next to `serial_reader.py` (MQTT or HTTP POST of the same JSON line). This Flutter app does not change. That is the point of keeping the hub in the middle.
