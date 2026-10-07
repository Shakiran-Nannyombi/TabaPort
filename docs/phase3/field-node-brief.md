# TabaPort field node: project brief and questions for Dr. Mary Nsabagwa

Prepared by the TabaPort team (Shakiran Nannyombi, Joel, Eric, Trevor, Sharif), October 2026.
To be read in ten minutes. The simulation referred to below runs on our laptop and we can show it during the meeting.

## 1. What TabaPort is

Kampala floods at the same thirty or so road crossings every rainy season: Clock Tower, Bwaise, Kalerwe, Mulago roundabout, Nalukolongo, and others. The water is not a surprise; the drains block, fill, and spill onto the road in a predictable order. What is missing is a signal that says "this drain is filling faster than it empties" early enough for a KCCA crew to clear the inlet or for traffic police to close a lane before the water reaches it.

TabaPort is a low cost water level node on the drain plus an offline dispatcher application on a laptop. The node measures water depth every half second, decides green, yellow or red against thresholds set from the drain's own depth, and reports. The laptop colours the road segment on a map, opens a job for a crew, and tracks it to closure. Nothing needs the internet; the whole thing runs on a bench with Wi‑Fi off. Reports from boda riders by phone are matched to the same map through a gazetteer of place names.

We are competing in the IEEE Response Quest Challenge (concept #5267). Phase 3 required a video of a real person using a working prototype; we filmed it on 7 October over a real campus drain with one node and the laptop.

## 2. What we have built and shown

**Generation 1 node (filmed).** One ESP32, one waterproof ultrasonic probe (JSN‑SR04T), a USB cable to the laptop. Firmware in C on ESP‑IDF: 2 Hz trigger, median of five with a 10 cm spike filter, JSON line per reading, three LEDs and a beep on red. The laptop recomputes everything from the raw echo distance so a wrong setting on the node cannot mislead the map.

**Dispatcher hub.** Flask and SQLite. Map screen (Leaflet, offline, 17 blackspots from Trevor's register), jobs screen (assign, acknowledge, en route, on site, close, false alarm, unacknowledged jobs flash after 3 minutes), live node screen (raw against filtered depth), settings (calibration and thresholds pushed to the node). If the node is silent for 10 seconds the segment goes grey, never green.

**Generation 2 field node (simulation).** This is what we would mount at a real blackspot and what we want your eyes on. It is built and running in the Wokwi simulator with every component wired and the firmware exercising each one. Section 3 lists them; section 4 lists what the simulator cannot tell us.

## 3. The field node, component by component

Pin map, firmware and circuit are in `prototype/sysmatic/field_node/`.

| Component | Purpose | In the simulation | Status |
|---|---|---|---|
| ESP32‑WROOM‑32 | controller, 2 cores, Wi‑Fi | ESP32 DevKit C | works |
| Ultrasonic probe | water depth from air gap | HC‑SR04 model (same Trig/Echo interface as the waterproof JSN‑SR04T) | works; a switch on the echo line simulates a cut probe cable |
| NTC thermistor | air temperature for speed of sound; 0.6 m/s per °C, about 1 cm error per 10 °C at 50 cm | 10 kΩ NTC on ADC | works; temperature compensation in firmware |
| Battery sense | node reports its own voltage; low battery flag below 3.5 V | potentiometer on ADC stands in for a 1:1 divider on a Li‑ion cell | works |
| MPU6050 | tilt from vertical; above 10° the reading is untrustworthy, state becomes unknown | I2C at 0x69 | works |
| DS1307 RTC | timestamps that survive power loss, offline | I2C at 0x68 | works; seeds itself from build time if stopped |
| 20×4 LCD | technician reads depth, temperature, battery, tilt, link and queue at the drain without a laptop | I2C at 0x27 | works |
| microSD | local log of every reading; store and forward queue for the uplink | SPI, FAT | works; queue drains when the link returns |
| Button | hold 1 s on a dry drain to set the zero; saved to flash | GPIO with pull‑up | works |
| Relay | drives a strobe or siren at the road on red | relay module and LED | works |
| LEDs and buzzer | local state and attention cue | GPIO and PWM | works |
| Uplink | send readings to the hub | Wi‑Fi to Wokwi's guest network, MQTT to a public test broker | works as a stand‑in; the real radio is the open question |

Behaviour worth seeing in the demo: drag the probe distance and watch green turn yellow and red with the strobe; cut the cable and watch it go to unknown rather than green; tilt the IMU and watch the same; drop the battery below 3.5 V; disconnect the network and watch the queue count climb on the LCD, then reconnect and watch it drain to zero.

What the simulator cannot model: a LoRa or GSM module, a solar panel and charge controller, the physical probe in spray and condensation, the enclosure, theft, and lightning. Those are the questions.

## 4. Questions for you

We have grouped them by theme. We do not expect answers to all of them in one meeting; we would value being pointed to the two or three that will hurt us most if we get them wrong.

### Uplink

1. For nodes at Bwaise, Kalerwe and Mulago reporting to a desk in the city centre, would you start with LoRa to a gateway we own, or GSM on each node? Your WIMEA‑ICT stations used a wireless sensor network; what would you choose today for ten to thirty nodes across Kampala?
2. What failure did you see most in the field link: antenna, power, SIM credit, network congestion during storms, or something else?
3. Is MQTT over GSM sensible for a 200 byte message every minute, or should we go to SMS as the transport with a USSD fallback as the Phase 2 concept proposed?
4. If LoRa: 433 or 868 MHz in Uganda, and is there a UCC licensing step we are not seeing?

### Power

5. The node draws about 80 mA awake on Wi‑Fi and under 1 mA in deep sleep. For a reading every minute during rain and every ten minutes when dry, what panel and battery would you size, and what charge controller have you trusted outdoors here?
6. How did you handle the long cloudy weeks? Did any station brown out, and what did the firmware do when it did?
7. Should we design for mains with battery backup where a street light pole is available, or assume no mains anywhere?

### The probe

8. Ultrasonic in a drain: spray, condensation on the transducer face, floating debris, and a 25 cm blind zone. Have you or your students used the JSN‑SR04T outdoors for months, and what killed it?
9. Would you move to a radar level sensor (A02YYUW class, UART) now, at roughly double the price, or prove ultrasonic first?
10. Our temperature compensation uses a thermistor in the box, not at the probe. Is that good enough or will the air column over the water differ enough to matter?
11. The tilt check uses a 10° limit. Is there a smarter way to detect a knocked or stolen probe?

### Enclosure and mounting

12. IP rating and material that survives a Kampala roadside: heat, sun, people. What worked for the weather stations and what did you have to replace?
13. Cable glands versus a sealed single cable run; conduit or not.
14. Theft and vandalism. Did you lose stations, and what reduced it: height, concealment, community involvement, KCCA branding?
15. Lightning. Any protection beyond a good ground, and did you see losses?

### Firmware and reliability

16. We reset on watchdog and keep the last calibration in flash. What reset and recovery policy did you settle on for unattended nodes?
17. We mark a segment unknown after 10 seconds of silence rather than showing the last known state. Do you agree, or would you keep a last known value with an age indicator?
18. Our spike filter is a median of five with a 10 cm gate. Is there a better cheap filter for a bottle floating under the probe?
19. Data accuracy: what is the honest accuracy to claim for an ultrasonic drain gauge, and how did you validate the weather stations against a reference?

### Programme

20. We have a working node and dispatcher and a video submitted. If you were advising us, what is the next experiment that produces evidence, not more features? One node at one real blackspot for a month?
21. Would you be willing to supervise this as a final year or research project, or point us to who should?
22. Is there any KCCA or UNMA contact you would introduce us to, and what should we have in hand before we meet them?

## 5. What we are asking of you

Thirty minutes to look at the field node design and tell us where it is naive. If you have a paper, a thesis, or a parts list from the weather station work that we should read before we buy anything, we will read it first.

## Appendix: running the simulation

```
cd prototype/sysmatic/field_node
pio run                                  # or: idf.py set-target esp32 && idf.py build
# VS Code: F1 -> Wokwi: Select Config File -> field_node/wokwi.toml
#          F1 -> Wokwi: Start Simulator
```

Serial output is one JSON line per reading:

```
{"node_id":"field_01","ts":"2026-10-07T16:20:05","raw_cm":44.9,"level_cm":5.1,"filtered":false,
 "state":"green","ok":true,"temp_c":24.0,"batt_v":3.96,"batt_low":false,"tilt_deg":0.0,"tilted":false,
 "sd":true,"queued":0,"link":"mqtt"}
```

The dispatcher hub reads the same line format, so a field node and the generation 1 node can both feed the map.
