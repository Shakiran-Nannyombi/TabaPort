# prototype — 7 October 2026 build

One ESP32 with a waterproof ultrasonic probe over a real drain, USB cable to a laptop running the dispatcher hub. Filmed for IEEE Response Quest Phase 3, concept #5267.

```
sysmatic/   node: ESP-IDF firmware, Wokwi circuit, pins   -> sysmatic/README.md
app/        hub: Flask + SQLite + Leaflet, three screens  -> app/README.md
```

Docs that go with it (in `docs/phase3/`): `bom.md` (what to buy), `wiring.md` (print and take to the drain), `thresholds-and-words.md` (bands, job wording, dispatcher lines), `rehearsal-and-shotlist.md` (Tuesday run, Wednesday shots, upload).

Quick start without hardware:

```bash
python3 -m venv .venv && .venv/bin/pip install -r prototype/app/requirements.txt
cd prototype/app && SERIAL_URL=fake ../../.venv/bin/python app.py
# http://localhost:5000/node -> Dev controls -> drag the level slider
```

With the ESP32 on USB, drop `SERIAL_URL=fake`.
