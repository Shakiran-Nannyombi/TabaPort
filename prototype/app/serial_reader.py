"""Serial reader thread. Reads JSON lines from the ESP32 and hands them to the state engine.

SERIAL_URL examples
  auto                      first /dev/ttyUSB*, /dev/ttyACM*, COM* found (default)
  /dev/ttyUSB0  or  COM5    real board
  rfc2217://localhost:4000  Wokwi simulator (development only)
  fake                      built-in simulator inside this process (no hardware, no Wokwi)
"""
import glob
import json
import os
import sys
import threading
import time
import urllib.request

import state_engine

status = {"url": None, "connected": False, "lines": 0, "last_error": None, "mode": None}
_port = None
_port_lock = threading.Lock()


def _autodetect():
    try:
        from serial.tools import list_ports
        for p in list_ports.comports():
            if any(k in (p.description or "") for k in ("CP210", "CH340", "USB", "UART")) or p.device.startswith(("/dev/ttyUSB", "/dev/ttyACM")):
                return p.device
    except Exception:
        pass
    for pat in ("/dev/ttyUSB*", "/dev/ttyACM*"):
        hits = sorted(glob.glob(pat))
        if hits:
            return hits[0]
    return None


def send(line):
    with _port_lock:
        if _port is not None:
            try:
                _port.write(line.encode())
            except Exception as e:
                status["last_error"] = f"write: {e}"


def uplink_base():
    """Laptop copies each real reading to the public hub. The public hub does not copy back."""
    if os.environ.get("RENDER"):
        return None
    raw = os.environ.get("TABAPORT_UPLINK", "https://tabaport.onrender.com").strip()
    if raw.lower() in ("", "off", "0", "none"):
        return None
    return raw.rstrip("/")


def _forward(line):
    base = uplink_base()
    if not base:
        return

    def send():
        try:
            req = urllib.request.Request(
                base + "/api/ingest",
                data=line.encode(),
                headers={"Content-Type": "application/json"},
                method="POST",
            )
            urllib.request.urlopen(req, timeout=4)
            status["uplink_error"] = None
        except Exception as e:
            status["uplink_error"] = str(e)

    threading.Thread(target=send, daemon=True).start()


def note_uplink():
    status.update(url="uplink", connected=True, mode="uplink", last_error=None)


def _handle_line(line):
    line = line.strip()
    if not line.startswith("{"):
        return
    try:
        msg = json.loads(line)
    except json.JSONDecodeError:
        return
    status["lines"] += 1
    state_engine.ingest(msg)
    if status.get("mode") != "fake":
        _forward(line)


def _real_loop(url):
    import serial
    global _port
    while True:
        target = _autodetect() if url == "auto" else url
        if not target:
            status.update(connected=False, last_error="no serial port found")
            time.sleep(2)
            continue
        try:
            port = serial.serial_for_url(target, baudrate=115200, timeout=1)
            with _port_lock:
                _port = port
            status.update(url=target, connected=True, last_error=None)
            print(f"[serial] connected {target}")
            time.sleep(0.5)
            send(state_engine.cfg_line("drain_01"))
            buf = b""
            while True:
                chunk = port.read(256)
                if not chunk:
                    continue
                buf += chunk
                while b"\n" in buf:
                    raw, buf = buf.split(b"\n", 1)
                    _handle_line(raw.decode(errors="ignore"))
        except Exception as e:
            status.update(connected=False, last_error=str(e))
            print(f"[serial] {e}; retrying", file=sys.stderr)
            with _port_lock:
                _port = None
            time.sleep(2)   # USB pulled -> watchdog marks unknown, we keep retrying


# ---------- in-process fake node (no hardware) ----------
fake = {"level_cm": 2.0, "auto": False, "spike_every": 0, "silent": False, "_n": 0}


def _fake_loop():
    import random
    status.update(url="fake", connected=True, mode="fake")
    h_sensor = state_engine.thresholds("drain_01")["h_sensor_cm"]
    while True:
        time.sleep(0.5)
        if fake["silent"]:
            status["connected"] = False
            continue
        status["connected"] = True
        fake["_n"] += 1
        if fake["auto"]:
            # slow rise then fall: a 3 minute pour scene
            t = (fake["_n"] * 0.5) % 180
            fake["level_cm"] = round(max(0.0, 22 * (1 - abs(t - 90) / 90)), 1)
        raw = h_sensor - fake["level_cm"] + random.uniform(-0.4, 0.4)
        filtered = False
        if fake["spike_every"] and fake["_n"] % fake["spike_every"] == 0:
            raw -= 25   # floating bottle under the probe
            filtered = True
        msg = {"node_id": "drain_01", "raw_cm": round(raw, 1), "filtered": filtered, "ok": True}
        _handle_line(json.dumps(msg))


def start():
    # The public hub has no USB port. It waits for the laptop to post real readings.
    if os.environ.get("RENDER"):
        status.update(url="uplink", connected=False, mode="uplink", last_error=None)
        return
    url = os.environ.get("SERIAL_URL", "auto")
    status["url"] = url
    status["mode"] = "fake" if url == "fake" else "serial"
    target = _fake_loop if url == "fake" else (lambda: _real_loop(url))
    threading.Thread(target=target, daemon=True, name="serial").start()
    state_engine.on_event(lambda ev: send(state_engine.cfg_line(ev["node_id"])) if ev["type"] == "node_hello" else None)
