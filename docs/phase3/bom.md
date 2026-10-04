# Bill of materials — 7 October prototype

One ESP32 node over one real drain, USB cable to a laptop. Everything else comes from the Arduino kit we already have.

Shop: Neriko Electronics, Wandegeya (+256 779 319 949). Call first to confirm stock of the JSN-SR04T. Prices are October 2026 street prices; expect ±20 %.

## Buy (Monday morning, Eric)

| # | Item | Qty | UGX each | UGX total | Notes |
|---|---|---|---|---|---|
| 1 | ESP32-WROOM-32 DevKit, 38-pin, CP2102, USB-C | 2 | 60,000 | 120,000 | Second board is the spare. If money is tight buy 1 (60,000) but a dead board on Tuesday ends the video. |
| 2 | JSN-SR04T waterproof ultrasonic (probe + driver board) | 1 | 25,000 | 25,000 | Blind zone 25 cm, lead ~2.5 m. Fallback: A02YYUW (UART, ~35,000) or the kit HC-SR04 under a plastic hood. |
| 3 | USB-C data cable, 2–3 m | 1 | 15,000–20,000 | 20,000 | Must be a **data** cable, not charge-only. Test on the laptop in the shop. |
| 4 | Insulating tape + cable ties (pack) | 1 | 5,000–8,000 | 8,000 | |
| 5 | Plastic lunch box with lid (control box) | 1 | 5,000–10,000 | 10,000 | Big enough for ESP32 + breadboard; drill one hole for cables. |
| 6 | Short plank or batten (~1 m) + 2 bricks or sandbag | 1 | 10,000–15,000 | 15,000 | Probe mount across the drain. Scrap wood is fine. |
| | **Total, two ESP32** | | | **~198,000** | |
| | **Total, one ESP32** | | | **~138,000** | |

## From the Arduino kit (no cost)

- Breadboard (half size is enough) and male–male / male–female jumper wires (need ~12)
- Resistors: 1 × 1 kΩ, 1 × 2 kΩ (or 2 × 1 kΩ in series), 3 × 220 Ω
- LEDs: 1 green, 1 yellow, 1 red
- Buzzer (active or passive, either works)
- HC-SR04 ultrasonic (backup sensor only; not waterproof)

## From home / campus

- Laptop with Python 3.10+ (the hub). Charger. Wi-Fi switched off during filming.
- Tape measure (5 m) for H_sensor and H_road
- Two buckets or a jerrycan for the pour if the rain does not come
- Phone for filming + a second phone or tripod
- Printed `wiring.md` one-pager and the calibration sheet

## Not buying (cut from scope)

Radar sensor, LoRa modules, GSM/SIM800, solar panel, battery, enclosure glands, SpatiaLite, any cloud service. These are generation 2 items for the field pilot, not the 7 October video.
