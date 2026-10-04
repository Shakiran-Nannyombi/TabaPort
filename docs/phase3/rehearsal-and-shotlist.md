# Tuesday rehearsal and Wednesday shot list

The IEEE rule: a real person using a working product, 2 to 5 minutes, no simulation, no stock footage, no AI video. Everything below is designed so that nothing on screen is faked and nothing needs the internet.

## Tuesday 6 Oct — rehearsal at the drain

People: Eric (drain, probe, crew role), Shakiran (laptop, dispatcher role), Trevor (camera 1 and site), Joel (water, timing, second camera), Sharif (hub, standby).

### Before leaving the room

- [ ] Laptop charged. Hub starts: `cd prototype/app && SERIAL_URL=auto python app.py`, page loads at http://localhost:5000 with **Wi-Fi off**
- [ ] `pip install -r requirements.txt` already done; no pip on site
- [ ] ESP32 flashed (`idf.py build flash` in `prototype/sysmatic/tank_node`); LEDs flash on power-up; JSON lines at 115200
- [ ] Spare ESP32 flashed too
- [ ] Print `wiring.md`, this sheet, and `thresholds-and-words.md`
- [ ] Two buckets or a 20 L jerrycan, tape measure, tape, cable ties, bricks
- [ ] Permission confirmed with the estates office / landlord; name of the person who said yes written here: ____________

### On site, in this order (allow 90 minutes)

1. Mount probe on plank across the drain. Straight down, ≥ 30 cm above max water, ≥ 30 cm from the wall.
2. Box on the bank, USB to laptop, cable taped down.
3. Measure **H_sensor** and **H_road** with the tape. Enter in Settings → "Compute and save". Note the yellow and red numbers: ____ / ____ cm.
4. Dry check: depth near 0, state **green**, chart moving. If depth is not near 0, re-aim the probe.
5. Wet check: pour a little, dip a ruler, compare with the laptop. Agree within 2 cm? ____
6. **Full run, timed:**
   - pour until **yellow** → job appears → assign Team A → Acknowledge
   - pour until **red** → beep from the box → red LED → action text changes
   - Eric "clears the inlet" (pull the plug / scoop) → level falls → sensor turns it green by itself
   - Crew on site → Close
   - pull USB → 10 s → **unknown**, grey on map → plug back → live again
   Time from first pour to red: ____ s. If over 90 s, use less water or a smaller section of drain.
7. Debris test: float a bottle under the probe for 2 s. Orange dot on the chart, state does not change. If it does, raise `SPIKE_CM` or window to 7 in both firmware and `state_engine.py`.
8. Reset data before each take: Live node → **Reset data** (only visible when `SERIAL_URL=fake`), or run `curl -X POST localhost:5000/api/dev/reset`.
9. Run the spoken lines twice with the camera rolling as practice. Watch the practice footage on the phone. Fix what is unclear.

### Known failure modes and the fix

| Symptom | Fix |
|---|---|
| Hub says "no serial port found" | `ls /dev/ttyUSB*`; add user to `dialout`; try the other USB cable (charge-only cables are the usual cause) |
| Depth jumps around by 5 cm when still | Probe too close to a wall, or less than 25 cm above water. Move it. |
| Depth reads 0 all the time | H_sensor is smaller than the real distance. Re-measure. |
| LEDs disagree with laptop | Save Settings again; it re-sends CFG to the node. |
| Board resets when LEDs light | Loose VIN or GND jumper; press the board in. |
| Everything dies on site | Spare ESP32. Fake mode only for checking the laptop, never for filming. |

## Wednesday 7 Oct — shot list

Two full takes. Phone horizontal, 1080p, no zoom while recording. Edit to 2 to 5 minutes. Target 3 min 30.

| # | Shot | Who is on camera | Length | Audio |
|---|---|---|---|---|
| 1 | Wide: the drain, the plank, the box, the cable running to the laptop on a table on the bank | Eric at the drain | 15 s | Shakiran voice: "This is a storm drain. This is the only sensor." |
| 2 | Close: probe over the water, LEDs green in the box | — | 10 s | "Fifty centimetres from probe to floor, measured with a tape." |
| 3 | Laptop screen: Live node, green, depth near zero, chart flat | Shakiran's hands | 10 s | "Wi-Fi is off. The laptop is the dispatch desk." |
| 4 | Pour begins (wide, both the drain and the laptop in frame if possible) | Joel pouring | 20 s | natural sound |
| 5 | Laptop: yellow. Jobs screen, job appears, assign Team A, Acknowledge | Shakiran | 25 s | dispatcher line 1 |
| 6 | Box: yellow LED. Back to pour | Eric / Joel | 10 s | natural |
| 7 | Laptop: red, beep audible, action text, Map screen shows red segment | Shakiran | 25 s | dispatcher line 2 |
| 8 | Eric clears the inlet / water drains; box LED goes yellow then green | Eric | 20 s | "Crew on site." |
| 9 | Laptop: level falling on the chart; Crew on site; Close. Segment is green because the sensor says so | Shakiran | 20 s | dispatcher line 3 |
| 10 | Pull USB on camera; laptop goes to unknown, grey; plug back | Eric pulls, Shakiran narrates | 20 s | dispatcher line 4 |
| 11 | Map: field report typed "Kalerwe mkt flooded" → matched to TP-03, segment yellow; Route crew button draws a route avoiding red | Shakiran | 25 s | "Reports from riders land on the same map. Routing avoids red." |
| 12 | Close on the team at the drain | all five | 10 s | end caption |

Rules while filming:

- Do not show Wokwi, a simulator, a slide, or any other software. Only the hub and the drain.
- Do not cut away during a state change. The judges need to see yellow become red on screen while water is poured.
- Keep the laptop clock visible; it proves continuity.
- No music over the dispatcher's voice.

## Upload (Wednesday evening, Thursday is the buffer)

- [ ] Export MP4, 1080p, under the portal's size limit (check the IEEE email for the limit before export)
- [ ] Watch the whole file once on a different device
- [ ] Upload to the Response Quest portal, concept #5267
- [ ] Screenshot the confirmation page and put it in `docs/phase3/`
- [ ] Portal closes 9 October. Do not wait for it.
