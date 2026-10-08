# Wiring and calibration one-pager (print this)

ESP32 DevKit + JSN-SR04T + 3 LEDs + buzzer on a breadboard, in a lidded box on the drain bank. Probe on a plank across the drain. USB-C cable to the laptop. Nothing else.

Print [tank-node-wiring.png](tank-node-wiring.png) and tick each jumper off as you put it in.

![Tank node wiring](tank-node-wiring.png)

## 1. Breadboard wiring

Do it in this order. USB **unplugged**.

**A. Power and trigger (4 jumpers)**

| Colour | From (ESP32) | To (JSN-SR04T driver) |
|---|---|---|
| Red | **VIN** (5 V from USB) | VCC |
| Black | GND | GND |
| Blue | GPIO5 (labelled D5 / IO5) | Trig |
| Orange | GPIO18, but only after the divider in B | Echo |

Do **not** put the sensor on 3V3. The JSN-SR04T will not fire, and Echo is still 5 V.

**B. Echo divider (the one that protects GPIO18)**

Echo is 5 V. ESP32 pins are 3.3 V. 1 kΩ then 2 kΩ to ground gives 3.3 V at the middle.

1. Orange jumper from **Echo** into one end of a **1 kΩ** (brown-black-red).
2. Other end of that 1 kΩ shares a breadboard row with a jumper to **GPIO18**.
3. From that **same row**, a **2 kΩ** (red-black-red) goes to the GND rail.

GPIO18 must sit in the middle of the two resistors. If you only have 1 kΩ parts, use two of them in series for the 2 kΩ to ground.

**C. LEDs and buzzer (each on its own wire, not chained)**

| ESP32 pin | Resistor | Component | Then | Orientation |
|---|---|---|---|---|
| GPIO25 | 220 Ω (red-red-brown) | green LED | GND | long leg toward the 220 Ω |
| GPIO26 | 220 Ω | yellow LED | GND | long leg toward the 220 Ω |
| GPIO27 | 220 Ω | red LED | GND | short / flat side to GND |
| GPIO14 | none | buzzer | GND | + or long leg to GPIO14 |

Checks before plugging in the laptop:

- [ ] No wire in the 3V3 pin that should be in VIN
- [ ] Echo goes to the divider, not straight to GPIO18
- [ ] LED long legs face the resistor side
- [ ] Nothing shorts across the ESP32 USB connector
- [ ] Press the board firmly into the breadboard; a loose VIN pin gives random resets

## 2. Pin table (for Shakiran's firmware and for the Wokwi diagram)

| Function | ESP32 pin | Notes |
|---|---|---|
| Sensor power | VIN | 5 V from USB; do **not** use 3V3 for JSN-SR04T |
| Ground | GND | all grounds common |
| Trig | GPIO5 | 20 µs pulse every 500 ms |
| Echo | GPIO18 | via 1 kΩ / 2 kΩ divider |
| Green LED | GPIO25 | 220 Ω |
| Yellow LED | GPIO26 | 220 Ω |
| Red LED | GPIO27 | 220 Ω |
| Buzzer | GPIO14 | one beep on red |

Avoid GPIO0, 2, 12, 15 (boot strapping) and GPIO34–39 (input only).

## 3. Mount rules (Eric)

- Probe face points **straight down**, within 5° of vertical.
- Probe face at least **30 cm above the highest water you will create**. Blind zone is 25 cm; closer than that reads garbage.
- At least **30 cm from a rough wall** or the echo bounces off the wall and not the water.
- Probe clamped with cable ties to the plank; plank weighted with bricks or a sandbag so a boot does not shift it mid-take.
- Driver board, ESP32 and breadboard **inside the lidded box** on the bank. Probe lead (2.5 m) runs through one hole. Tape the hole.
- USB cable taped down along the ground so the camera and feet do not pull it. A pulled cable is a real failure mode and the hub shows **unknown** for it, which is a good shot, but do it on purpose, not by accident.

## 4. Calibration sheet (fill in on site, dry, with the tape)

| Measure | Symbol | Value (cm) |
|---|---|---|
| Probe face → drain floor (invert) | H_sensor | ____ |
| Road / path edge → drain floor | H_road | ____ |
| Dry echo reading from the hub (should ≈ H_sensor) | d_dry | ____ |
| Difference H_sensor − d_dry (if > 3 cm, re-aim the probe) | | ____ |

Water depth shown on the laptop = H_sensor − measured echo. Check once with a ruler in the water during the first pour; it should agree within 2 cm.

Thresholds (Joel's rule, see `thresholds-and-words.md`):

| Band | Rule | Value (cm) |
|---|---|---|
| Yellow | 50 % of H_road | ____ |
| Red | 80 % of H_road, or anything within 10 cm of the road edge | ____ |

Enter H_sensor, H_road and the two thresholds on the hub **Settings** page (or use "Compute and save" there). Saving pushes them to the ESP32 so the LEDs on the box agree with the laptop.

If the drain is shallower than 40 cm use the tank fallback numbers: yellow 8 cm, red 15 cm.

## 5. Bring-up order (Tuesday, 30 minutes)

1. Breadboard on the desk, no probe in the drain yet. Plug USB. Three LEDs flash in turn and one beep = firmware alive.
2. `idf.py -p /dev/ttyUSB0 monitor` (115200): one JSON line every 0.5 s. Wave a hand under the probe; `raw_cm` drops. Ctrl+] to leave before starting the hub, since only one program can hold the port.
3. Start the hub: `python app.py` in `prototype/app`. Open http://localhost:5000/node. The chart moves.
4. Carry the box to the drain. Mount probe. Measure H_sensor and H_road. Enter them in Settings.
5. Dry reading should be near zero depth and state green.
6. Pour one bucket. Watch yellow. Pour the second. Watch red, hear the beep, see the job appear.
7. Pull the USB on purpose. Ten seconds later the map goes grey. Plug it back. Back to the live state.
