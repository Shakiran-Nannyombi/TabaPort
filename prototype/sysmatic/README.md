# prototype/sysmatic — node hardware and firmware

One ESP32 + one waterproof ultrasonic over a real drain. ESP-IDF, plain C. Wiring is in [`../../docs/phase3/wiring.md`](../../docs/phase3/wiring.md). Parts and prices in [`../../docs/phase3/bom.md`](../../docs/phase3/bom.md).

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
