# prototype/sysmatic — node hardware and firmware

Two nodes, both ESP-IDF in plain C, both build with `idf.py` or `pio run`:

| | `tank_node/` | `field_node/` |
|---|---|---|
| Purpose | Generation 1, filmed 7 Oct over the real drain | Generation 2 reference design, simulation only, for review with Dr. Nsabagwa |
| Parts | ESP32, ultrasonic, 3 LEDs, buzzer | + thermistor, battery sense, MPU6050 tilt, DS1307 RTC, 20x4 LCD, microSD, dry-zero button, relay + strobe, Wi-Fi/MQTT uplink |
| Docs | [`wiring.md`](../../docs/phase3/wiring.md), [`bom.md`](../../docs/phase3/bom.md) | [`field-node-brief.md`](../../docs/phase3/field-node-brief.md) (components, what the sim can't model, questions) |

## tank_node

One ESP32 + one waterproof ultrasonic over a real drain.

```
tank_node/
  CMakeLists.txt        ESP-IDF project
  platformio.ini        same project built with PlatformIO (framework = espidf)
  sdkconfig.defaults    esp32 target, 4 MB flash, quiet logging on UART0
  main/
    CMakeLists.txt
    main.c              firmware
  diagram.json          Wokwi circuit: ESP32 + ultrasonic + divider + 3 LEDs + buzzer
  wokwi.toml            Wokwi config; serial exposed on TCP 4000 for the hub
```

Built and checked with **ESP-IDF v6.0.1** (installed through EIM at `~/.espressif`). Any 5.x should also work; the driver component names in `main/CMakeLists.txt` (`esp_driver_gpio`, `esp_driver_uart`, `esp_driver_ledc`) exist from 5.3 onward.

## Build and flash the real ESP32

```bash
# activate the IDF environment (EIM install)
source ~/.espressif/tools/activate_idf_v6.0.1.sh      # or: . $IDF_PATH/export.sh

cd prototype/sysmatic/tank_node
idf.py set-target esp32          # once
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor   # Ctrl+] leaves the monitor
```

Hold **BOOT** on the board if flashing says "Connecting…". On Linux, if the port is not writable: `sudo usermod -aG dialout $USER`, then log out and in. On Windows the port is `COMx`.

Or with PlatformIO (VS Code extension or `pio` CLI), from the same folder:

```bash
pio run                 # build  -> .pio/build/esp32dev/firmware.bin
pio run -t upload       # flash
pio device monitor      # 115200
```

`platformio.ini` sets `src_dir = main` and `framework = espidf`, so it compiles the same `main/main.c` and `sdkconfig.defaults`. For Wokwi with a PlatformIO build, change `wokwi.toml` to `firmware = ".pio/build/esp32dev/firmware.bin"` and `elf = ".pio/build/esp32dev/firmware.elf"`.

In the monitor you should see one JSON line every 0.5 s. Wave a hand under the probe; `level_cm` and the LEDs change.

## What the firmware does (`main/main.c`)

- Trigger every 500 ms, echo timed with `esp_timer` (30 ms timeout), d = t × 0.0343 / 2
- Moving median of 5; if raw is more than 10 cm from the median the reading is marked `filtered` and the median is used
- `level_cm = h_sensor_cm − distance`, clamped at 0
- Green / yellow / red LEDs on GPIO25 / 26 / 27 from the same thresholds the hub uses; one 2 kHz beep on GPIO14 when red starts
- Five missed echoes in a row → all three LEDs blink and the line says `"ok":false`
- Listens on UART0 for `CFG h_sensor=… yellow=… red=…` from the hub and echoes the new values back
- No Wi-Fi, no sleep, no LoRa

Serial line formats are documented in [`../app/README.md`](../app/README.md).

## Calibrate on site (Tuesday)

Measure with the tape, dry:

- `H_sensor` = probe face → drain invert (floor)
- `H_road` = road/path edge → drain invert

Type them into the hub **Settings** page. Saving sends `CFG` to the node so box LEDs and laptop agree. Defaults without CFG: `h_sensor=50 yellow=8 red=15` (rain-day tank numbers).

## Wokwi (Sunday–Monday only)

Install the Wokwi extension in VS Code, run `idf.py build`, then F1 → **Wokwi: Start Simulator**. `wokwi.toml` points at `build/flasher_args.json`, so bootloader, partition table and app are all loaded. Click the ultrasonic part and drag its distance slider: LEDs and serial JSON change. The hub reads the simulator with `SERIAL_URL=rfc2217://localhost:4000`.

**Do not film Wokwi.** IEEE forbids simulated interaction. The video is the real probe over the real drain.

## Pins

| ESP32 | Goes to |
|---|---|
| VIN (5 V) | JSN-SR04T +5V |
| GND | JSN-SR04T GND, R2 bottom, LED cathodes, buzzer − |
| GPIO5 | JSN-SR04T Trig |
| GPIO18 | Echo via 1 kΩ series; 2 kΩ from GPIO18 to GND |
| GPIO25 / 26 / 27 | 220 Ω → green / yellow / red LED |
| GPIO14 | buzzer + |

Probe: pointing straight down, centred, **≥ 30 cm above the highest expected water**, ≥ 30 cm from a rough wall. Control board stays in the lidded box on the bank.

## field_node

```
field_node/
  main/main.c           reading loop, state, LCD, log + queue, JSON line
  main/i2c_devices.c    LCD 20x4 (PCF8574), DS1307, MPU6050
  main/sd_log.c         FAT on SPI microSD; append + store-and-forward queue
  main/uplink.c         Wi-Fi station + MQTT (stand-in for LoRa / GSM)
  main/idf_component.yml  pulls espressif/mqtt from the component registry (IDF 6.1+)
  diagram.json          full circuit, labelled
  wokwi.toml            serial on TCP 4001; microSD filled from sdcard/
  sdcard/               initial card contents
```

Pins: Trig 16, Echo 17 (1k/2k divider, switch = cable), I2C SDA 21 / SCL 22, SD SCK 18 / MOSI 23 / MISO 19 / CS 5, battery ADC 34, NTC ADC 35, button 4, relay 13, LEDs 25/26/27, buzzer 14.

Run: `pio run` (or `idf.py build`), then in VS Code F1 → **Wokwi: Select Config File** → `field_node/wokwi.toml` → **Wokwi: Start Simulator**. Things to try: drag the probe distance; flip the cable switch (→ unknown); drag the MPU6050 accelX to 0.3 (→ tilt → unknown); turn the battery pot down (→ `batt_low`); hold the blue button 1 s on a "dry" distance (→ dry zero saved); watch the LCD queue count when the uplink is down and drain when it returns.

Not simulated and therefore on the question list: LoRa/GSM module, solar + charge controller, the probe in spray, enclosure, theft, lightning.

Flash use is ~965 KB. `sdkconfig.defaults` selects the 1.5 MB single-app partition table for real hardware; Wokwi loads the app-only `.bin` with its own table.
