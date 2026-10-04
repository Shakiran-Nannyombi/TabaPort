# Thresholds, job wording and the dispatcher's lines (Joel)

This is what the laptop says and does, in plain words, so the dispatcher in the video is not inventing language on camera.

## 1. Water level bands

Depth = H_sensor − echo distance, in cm above the drain floor. H_road is the depth at which water reaches the road or path edge. Both are measured on site and typed into Settings.

| Band | Default rule | Tank fallback (drain < 40 cm) | What it means for the road |
|---|---|---|---|
| Green | depth < 50 % of H_road | < 8 cm | Drain is working. Nothing to do. |
| Yellow | depth ≥ 50 % of H_road | ≥ 8 cm | Drain is filling faster than it empties. Inlet is probably blocked. Send someone now so it does not become red. |
| Red | depth ≥ 80 % of H_road, **or** within 10 cm of the road edge | ≥ 15 cm | Water will be on the road in minutes. Close the lane. Crew must desilt the inlet. |
| Unknown | no reading for 10 s | same | We do not know. Never show green when we do not know. Treat as yellow for routing. |

Example for a 60 cm deep drain with the probe 50 cm above the floor: yellow at 30 cm, red at 48 cm (80 %) which is also within 10 cm of the edge (50 cm). The Settings page computes these from H_road.

Rise rate (cm per minute over the last 60 s) is shown but does not change the band in generation 1. A rise rate above 5 cm/min during a storm is the field signal that the inlet is blocked rather than the drain just being full; that is a generation 2 rule once we have real storm data.

Why 50 and 80: KCCA drains are designed with freeboard. Half full during rain is normal only if the water is moving. Half full and rising means the outlet or inlet is restricted. Eighty percent leaves roughly the time it takes a crew to reach a CBD blackspot from the Clock Tower depot in traffic (10 to 15 min) before the road is wet.

## 2. Job action wording (what appears on the Jobs screen)

| Trigger | Job action text |
|---|---|
| Transition to yellow | **Send drainage crew to check inlet** |
| Transition to red | **Ask traffic police to close lane; send drainage crew to desilt inlet** |
| Human report, no sensor | **Send drainage crew to check inlet** (segment marked yellow from the report, never red without a sensor or a crew on site) |
| Opened by hand | dispatcher types the action |

Job statuses, in order: open → acked → en_route → on_site → closed. Side exit at any step: false_alarm with a reason.

Rules the dispatcher follows:

- Acknowledge within 3 minutes. After 3 minutes an unacknowledged job flashes red.
- Assign a crew before or at acknowledgement.
- Closing a job does **not** make the road green. Only the sensor makes it green. If the crew clears the inlet the water drops and the sensor turns it green itself.
- False alarm reasons we expect: "debris under probe", "crew found drain clear", "test pour".

Crews in the demo config: Team A - Central, Team B - Kawempe, Team C - Rubaga, Traffic Police. Change in Settings → `crews`.

## 3. Dispatcher role and caption for the video

Role: **KCCA drainage dispatcher**, duty desk. One person, one laptop, Wi-Fi off. They do not touch the drain; they respond to what the map tells them.

Caption shown on screen at the start of the dispatcher scene:

> *TabaPort dispatcher desk. Offline laptop, one drain sensor on USB. Everything on screen is live.*

Spoken lines, kept short. The dispatcher says what they see and what they do:

1. On yellow: "Campus drain is at thirty centimetres and rising. That's yellow. I'm sending Team A to check the inlet." → clicks **assign Team A**, **Acknowledge**.
2. On red: "Forty eight. Red. Water is ten centimetres from the path. I'm asking traffic to close the lane." → reads the action line out loud.
3. When the crew (Eric, on camera at the drain) clears the inlet and the level drops: "Level is dropping. The sensor will turn it green on its own. I close the job when Team A reports clear." → **Crew on site**, **Close**.
4. On the deliberate USB pull: "I've lost the node. It goes grey, not green. I don't guess." → plugs back in.

Caption at the end:

> *One node, one drain, one dispatcher. No internet, no SMS, no cloud. Generation 1.*

## 4. Config defaults in the database

```
yellow_cm.drain_01 = 8      (tank fallback; replaced on Tuesday from H_road)
red_cm.drain_01    = 15
h_sensor_cm.drain_01 = 50
h_road_cm.drain_01   = 60
unknown_after_s    = 10
escalate_after_s   = 180
```
