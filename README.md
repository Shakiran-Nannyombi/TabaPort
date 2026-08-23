# TabaPort

Offline-first edge intelligence and dispatch for Kampala urban flash floods.

**Status:** IEEE Response Quest Challenge Phase 3 (concept #5267). Research and planning. No working software or hardware in this repo yet.

**Author:** Shakiran Nannyombi  
**Phase 3 deadline:** 9 October 2026 (2–5 minute video of a real person using a working prototype)

---

## What it is

When heavy rain hits Kampala, culverts overflow and roads at Clock Tower, Bwaise, and Kalerwe can close in minutes. Cellular networks often congest or drop. Field crews then cannot see which drain is rising, which road is still passable, or where to send the next truck.

TabaPort is meant to run at the edge, not in the cloud:

1. A low-cost node measures water level in a culvert.
2. A laptop (or later a field hub) keeps an offline map.
3. Roads show **green / yellow / red**. A silent node shows **unknown**, not safe.
4. A short ranked list tells a dispatcher where to go first.
5. If a segment goes red, routing avoids it.

It does not stop flooding. KCCA’s drainage works move water. TabaPort tells responders where the water is.

---

## Current plan

Do this in order. Do not skip ahead to KCCA.

1. Read and write a short technical note for **Dr. Mary Nsabagwa** (Makerere CoCIS, embedded systems / WSN).
2. Buy and build **one lab node** (tank + laptop) after she comments.
3. Film the IEEE Phase 3 video with a real person using the interface.
4. Only then approach KCCA Engineering.

---

## Repository

```
docs/
  phase2/     IEEE Phase 2 submission (PDF)
  phase3/     Phase 3 notes and templates
LICENSE       MIT
```

Firmware, dashboard, and hardware BOM will land in this repo when generation 1 exists.

---

## Generation 1 (intended)

Not built yet. Planned parts are local and cheap:

- ESP32
- Waterproof ultrasonic (e.g. JSN-SR04T)
- Laptop as the hub
- Test tank (no public drain)

LoRa, solar, and a city-scale SMS loop are later. Do not promise cell-broadcast alerts to every phone in 2 km.

---

## License

[MIT](LICENSE)
