# Dr. Mary Nsabagwa — briefing before you approach her

Use this file only for the lecturer meeting. Keep KCCA and the IEEE video plan in `advice.md`.

**What she reads:** you write a short note from `nsabagwa-report-template.md` after you have done the reading. Do not send this briefing file to her.

**Order:** Nsabagwa → one lab node → IEEE video → KCCA.

**Published name:** Dr. Mary **Nsabagwa** (not Nsawbagwa). Use this spelling in every email and on the printed page.

---

## 1. Who she is

| | |
|---|---|
| Role | Lecturer, Department of Networks, Makerere CoCIS |
| Faculty page | https://cocis.mak.ac.ug/faculty/networks/mary-nsabagwa/ |
| Email | mnsabagwa@cit.ac.ug |
| Office | CoCIS Block A, Room 301 |
| Hours | 08:00–17:00 (listed on faculty page) |
| Teaching | Embedded systems, C, Java, Internet of Things |
| Research | Embedded systems and Wireless Sensor Networks; IoT for community problems |
| Supervision | More than 50 students |

**Degrees:** PhD Computer Science (Makerere); MEng Computer Science and Technology (Hunan University, China); BSc Computer Science (Makerere).

She has published 20+ papers and worked across electrical engineering, meteorology, veterinary medicine, and public health. She also lists **requirements elicitation** as a strength. She will notice if you have done homework, and she will notice if you have not.

A mobile number is on her public faculty page. Use **email or Room 301 first**. Do not call before she has replied.

---

## 2. Why she is the right person for TabaPort

She already built **outdoor, low-cost, Uganda-deployed wireless sensor networks** under WIMEA-ICT (automatic weather stations). Those nodes sit in the rain, run on solar and batteries, send data over a WSN, and have to stay accurate enough for operational use. That is the same class of problem as a culvert node in Bwaise.

She is not being asked to judge the IEEE challenge or to introduce you to KCCA. She is being asked whether a **first-generation flood node** is technically sound.

### How her work maps onto TabaPort

| Her work | Your Phase 2 claim | What she will test you on |
|---|---|---|
| Affordable AWS for Uganda (WIMEA target about USD 1,000 per station vs imported stations) | “Under US$55 per node” | Is that a lab board, or a node that survives rain, theft, and 3 months without you? |
| Condition monitoring of WSN weather stations | Moving-median filter on the ESP32 | How do you know the *sensor itself* is lying, not just filtering junk? |
| Network densification for Uganda weather stations | 69 KCCA blackspots | Where do you put the first 3 nodes, and why not 69 at once? |
| Rainy-season / night battery on WIMEA AWS | Solar + ESP32 sampling every 30 s | 30 s may kill the battery. She will ask for a power budget. |
| Requirements elicitation | “KCCA needs this” | Have you talked to a drainage engineer yet? Honest answer for now: **not yet.** |

Pattern from her other work (AdEMNEA bee/fruit-fly IoT; 2023 “Eye for the Blind” student team): community problem + student-buildable hardware + field reliability. TabaPort fits if you do not oversell it.

---

## 3. What to read before Room 301

Do this in order. Do not try to read 20 papers.

### Must read (1–2 hours)

Nsabagwa, M., Byamukama, M., Kondela, E., & Otim, J. S. (2019). Towards a robust and affordable Automatic Weather Station. *Development Engineering*, 4, 100040.  
https://doi.org/10.1016/j.deveng.2018.100040  
(open access)

She evaluated a first-generation AWS and said the next version had to improve four non-functionals: **reliability, data accuracy, power consumption, and data transmission**. Cost only matters after those four.

While you read, fill the table in section 5. That table is your meeting ticket.

### Second read (45 minutes)

Nsabagwa, M., Sansa Otim, J., Nyongarwizi Akol, R., Ninsiima, G., Mwesigye, R., Byamukama, M., & Pehrson, B. (2018). Condition Monitoring for Wireless Sensor Network-Based Automatic Weather Stations. *EAI Endorsed Transactions on Internet of Things*, 4(14), e4.  
https://publications.eai.eu/index.php/IoT/article/view/661

She looks at arrival rate and packet content to tell whether a **sensor**, a **node**, or a whole **station** is sick. For floods this matters more than weather, because a dead node looks like “no flood.”

Sentence to have ready:

> If a culvert node goes silent, TabaPort should treat that as “unknown,” not “safe.”

### Optional (if you have another hour)

- Nsabagwa, M., Byamukama, M., Otim, J. S., & Okou, R. (2016). Network densification strategies for Automatic Weather Stations: Challenges and opportunities for Uganda. IST-Africa. https://doi.org/10.1109/istafrica.2016.7530633
- Faculty profile: https://cocis.mak.ac.ug/faculty/networks/mary-nsabagwa/
- WIMEA-ICT context: https://wimea-ict.net/?p=1664

---

## 4. Hardware you must be able to defend

You do not need to have bought parts yet. You do need to know why each part is on the list.

| Part | Local ballpark (Kampala) | Why it is on the list | Weakness to admit |
|---|---|---|---|
| ESP32 DevKit | ~UGX 60,000 (Neriko, Wandegeya) | Wi-Fi + BLE, cheap, local stock, can run a moving-median filter on-chip | Higher idle current than a bare module in deep sleep |
| JSN-SR04T waterproof ultrasonic | ~UGX 25,000 | Splash-proof probe, 5 V, common for tank/level demos | Not designed to sit in dirty water full of plastic; foam and debris confuse it |
| LoRa module (e.g. E32 433 MHz) | ~UGX 80,000 | Field fallback when cellular is down | Needs a gateway; you do not have one yet |
| Box, wires, protoboard | ~UGX 20,000–40,000 | First-generation enclosure | Not IP-rated for a live culvert |
| Power bank / small solar + Li-ion | ~UGX 40,000–80,000 | Demo power | No measured power budget yet |
| Laptop as hub | UGX 0 extra | Phase 3 edge hub; skip Raspberry Pi for now | Not a rugged field appliance |

**Phase 3 first node (recommended):** ESP32 + JSN-SR04T + box ≈ **UGX 120,000**, plus power. Laptop is the hub.

**Phase 2 claim to treat as untested:** sample every 30 seconds. Be ready to drop to 1–5 minutes, or wake only when level rises, if she says the power budget is wrong.

**Do not promise her (or later KCCA):** city-wide cell broadcast SMS, ultrasonic accuracy inside a silted public drain, or a finished solar node.

---

## 5. Four-box table — fill this from her 2019 paper

Copy this onto the printed one-pager. Update the “Status” column after you have actually tested something. As of 23 August 2026, none of the hardware has been built. Everything below is a **Phase 2 claim**, not a measurement.

| Non-functional (her language) | What Phase 2 claimed | Status now | Honest next test |
|---|---|---|---|
| **Reliability** | Cellular → SMS → LoRa/BLE fallback; node keeps working if the cloud dies | Not tested. No node, no radio fallback, no soak test. | One node running 72 hours on a bench, including a Wi-Fi drop. Silent node must show **unknown**, not green. |
| **Data accuracy** | Moving-median filter on ESP32 drops plastic/debris spikes; baseline recalibration when there is no rain | Filter is specified, not implemented. Ultrasonic in dirty water is unproven. | Tank test: clean water, then float bottles/bags, then compare raw vs median. Write the window size (e.g. 5 or 7 samples). |
| **Power consumption** | Solar-powered; sample every 30 s | No power budget. 30 s is likely too aggressive. | Measure ESP32 + sensor current: awake, sleep, transmit. Recalculate samples per day on a 2000–3000 mAh pack. |
| **Data transmission** | Wi-Fi / GSM under normal conditions; SMS if towers degrade; LoRa/BLE if cellular collapses; hub within ~500 m | Not tested. Laptop-only path planned for Phase 3. | Lab: USB/Wi-Fi serial from ESP32 to laptop. Ask her whether LoRa or GSM should be generation 2. |
| **Cost** (after the four above) | Under US$55 per production node | Handmade Kampala node will land closer to UGX 250,000–350,000 with solar, box, and radio | Buy only the UGX ~120,000 first-generation kit until she signs off the sensor choice |

---

## 6. Printable one-pager (one side of A4)

Print **two copies**. Leave one.

---

**TabaPort — first-generation node (advice request)**  
Shakiran Nannyombi · IEEE member · Response Quest #5267 · Phase 3 due **9 October 2026**

**One sentence.** Culvert water level → offline hub → red / yellow / green roads → ranked jobs for a KCCA-style dispatcher.

**Why this meeting.** I am not asking for funding or a KCCA introduction. I want the first node to be a first-generation prototype in the same sense as your 2019 AWS paper: cheap, local, and honest about reliability, accuracy, power, and radio.

**Phase 3 need.** A real person using a working interface on camera (2–5 minutes). One physical node in a test tank plus a laptop hub is enough. No public drain install.

**Proposed first node.** ESP32 + JSN-SR04T ultrasonic + box ≈ UGX 120,000. Laptop as hub. LoRa/GSM later.

**Four non-functionals (not yet tested).**

| | Claim | Status |
|---|---|---|
| Reliability | Offline hub if the cloud dies; silent node ≠ safe | Not tested |
| Accuracy | Moving median ignores floating plastic | Specified only |
| Power | Solar; sample every 30 s | No budget; 30 s may be wrong |
| Radio | Wi-Fi / SMS / LoRa fallback | Laptop Wi-Fi only for now |

**Questions**

1. For generation 1, is waterproof ultrasonic acceptable in a **tank**, or should I start with a pressure sensor so the field version is not a dead end?
2. In your AWS work, what actually killed nodes: power, radio, water ingress, or vandalism? Which should I design for first?
3. Is 30-second sampling realistic on a small solar pack in Kampala’s rainy weeks, or should the node sleep and wake on a rise threshold?
4. For 3 later sites (Clock Tower, Bwaise, Kalerwe), LoRa to a laptop hub, or GSM/SMS first because I do not have a LoRa gateway?
5. How should I detect a fouled or drifted ultrasonic so the dashboard does not show green when the culvert is full?
6. If I build one node this month, what would you want in a lab log before you would call it working?

---

## 7. Email to send (or skip if you already know her from class)

If you already attend her class or can walk to Block A, Room 301 during office hours, a short in-person ask is enough. Use email if you need a written trail.

**To:** mnsabagwa@cit.ac.ug

**Subject:** Advice on a first-generation flood-sensor node (IEEE Phase 3)

Dear Dr. Nsabagwa,

I am Shakiran Nannyombi. I take your embedded systems work seriously, and I would like 20 minutes of advice before I buy hardware.

My project, TabaPort, is an offline-first flood dispatch tool for Kampala. IEEE selected it for Phase 3 of the Response Quest Challenge. The submission is a 2–5 minute video of a real person using a working prototype, due 9 October. I am not approaching KCCA yet.

I have read your 2019 paper on a robust, affordable automatic weather station. I want the first TabaPort node to be a first-generation prototype in the same sense: cheap, local, and honest about reliability, accuracy, power, and radio.

Proposed first node: ESP32 + waterproof ultrasonic in a test tank, laptop as the hub, later LoRa/GSM. I am unsure whether ultrasonic is the right sensor once this leaves the lab, and whether 30-second sampling is realistic on a small solar pack.

I can come to Block A, Room 301 at your convenience this week or next. I will bring a one-page note and a parts list, not a request for funding.

Kind regards,  
Shakiran Nannyombi  
[programme, year] | IEEE member | TabaPort #5267  
[phone] | [email]

---

## 8. How to run the 20 minutes

1. One minute of context. IEEE Phase 3. Offline flood dispatch. No KCCA yet.
2. Put the one-pager on the table. Do not read it aloud.
3. Ask question 1 (sensor choice). Let her talk.
4. Ask questions 2–6 only if time remains.
5. Write down what she wants in the lab log. That becomes your build checklist.
6. Ask if you may come back in two weeks with the tank demo. Stop there.

### Do not ask in this meeting

- “Can you introduce me to KCCA?”
- “Can you fund this?”
- “Is my IEEE idea good?”
- A lecture on the Drainage Master Plan (one sentence is enough)
- For her to pick the entire BOM while you have written no firmware

---

## 9. After she talks

Write her answers into this file the same day, under the headings below, so you do not lose them.

**Sensor she recommends:**

**Sample interval / sleep strategy:**

**Radio for generation 1 vs 2:**

**What “working” means in the lab log:**

**Parts to buy this week:**

**Parts to wait on:**

Only after that lab node exists should you use `advice.md` to approach KCCA.
