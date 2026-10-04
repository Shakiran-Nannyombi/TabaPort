# TabaPort — concept note

**Offline-first flood intelligence and dispatch for Kampala**  
MIIC Technology Entrepreneurship Seed (TES) | IEEE Response Quest Challenge Phase 3, concept #5267  
Lead: Shakiran Nannyombi | Status: generation 1 in build (lab node + dispatcher interface)

---

## One sentence

A culvert sensor feeds an offline hub that colours roads green / yellow / red and ranks where a KCCA crew should go first — even when the mobile network is down.

## Problem

Kampala flash floods close Clock Tower, Bwaise, Kalerwe and other blackspots in minutes. Networks congest or drop. Drainage crews, emergency teams, boda riders and commuters then cannot see which drain is rising, which road is passable, or where to send the next truck. Dispatch today is radio, WhatsApp and people standing in water.

KCCA has 69 flood blackspots on a 362 km drainage network and a multi-year civil-works programme (about UGX 768 billion through 2030/31). UNDP cites about US$50 million a year in flood damage. Construction moves water. It does not tell responders **where the water is** while channels are still being built, or after they are built when waste and silt block them.

TabaPort does not stop flooding. It reduces blind deployment and people driving into water.

## Solution (technology at the core)

| Layer | What it does |
|---|---|
| IoT node | ESP32 + waterproof ultrasonic (e.g. JSN-SR04T) measures culvert water level. A moving-median filter ignores floating plastic. Target production cost under US$55 per node. |
| Edge hub | Laptop now; rugged field hub later. Local map. No cloud required. |
| Dispatch | Roads: green / yellow / red. Silent node = **unknown**, not safe. Ranked job list (rise rate + hospital / flyover / dense neighbourhood). Routing avoids flooded segments. |
| Human report | Optional USSD/SMS from a boda who saw a blockage, until a sensor confirms. Phone numbers stripped at the doorway. |

Generation 1 (in progress): one node in a **test tank**, not a public drain, plus a dispatcher screen a real person can use. LoRa, solar and a city SMS loop come later. Cell-broadcast alerts to every phone are a UCC/telco path, not a student Pi.

## Who it is for

Primary user: KCCA drainage and emergency dispatchers already pre-positioned when rain starts.  
Secondary: field engineers at the 69 blackspots; later, a public “avoid this road” view.

## Why TES, why now

TES is the right home because this is still an early-stage hardware-plus-software build: idea proven in IEEE Phase 2, prototype in progress for Phase 3 (video of a real person using the product, due 9 October 2026). We need lab access, mentorship on reliability / power / radio, and light catalytic support to finish generation 1 and prepare a 30-day trial ask — not city-wide rollout.

## Honest limits

- Not competing with Nakivubo rehabilitation or new culverts. It sits on top of that work.
- No public-drain install without written KCCA permission.
- City-scale accuracy in dirty culverts needs a field trial before anyone should trust it for live dispatch.

## Next 90 days

1. Finish the lab node and offline dispatcher interface.  
2. Technical review with an embedded / WSN adviser (Makerere CoCIS).  
3. IEEE Phase 3 demo video.  
4. Then a 20-minute briefing with KCCA Engineering (reaction, not funding): how reports reach crews today; which 5 of 69 blackspots to instrument first; whether a non-invasive demo on KCCA premises is acceptable.
