Her published name is **Dr. Mary Nsabagwa** (not Nsawbagwa). Use that spelling in every email. She is a very good first stop, and you should see her **before** KCCA.

She is a Lecturer in the **Department of Networks**, Makerere CoCIS. Faculty page: [Mary Nsabagwa](https://cocis.mak.ac.ug/faculty/networks/mary-nsabagwa/). Email: `mnsabagwa@cit.ac.ug`. Office: **CoCIS Block A, Room 301**. Hours listed as 08:00–17:00.

PhD in Computer Science (Makerere), MEng from Hunan University, BSc Computer Science (Makerere). She teaches embedded systems, C, Java, and IoT, and has supervised 50+ students. Her real strength for you is not “she knows chips.” It is that she has already built **outdoor, low-cost, Uganda-deployed wireless sensor networks** and then written down why they fail.

## Why she is the right person for TabaPort

Her main research line is **WIMEA-ICT automatic weather stations**: cheap nodes that sit in the rain, run on solar and batteries, send data over a wireless sensor network, and have to stay accurate enough for UNMA-style use. That is the same class of problem as a culvert node in Bwaise.

The paper to read first is hers as corresponding author:

[Nsabagwa et al., “Towards a robust and affordable Automatic Weather Station,” *Development Engineering*, 2019](https://doi.org/10.1016/j.deveng.2018.100040) (open access).

She evaluated a first-generation AWS and said the next version had to improve four things: **reliability, data accuracy, power consumption, and data transmission**. Cost only matters after those four. That is exactly the conversation you need before you buy an ESP32 and a tank.

Her other work that maps onto TabaPort:

| Her work | Your Phase 2 claim | What she will test you on |
|---|---|---|
| Affordable AWS for Uganda (~USD 1,000/station in WIMEA, vs imported stations) | “Under US$55 per node” | Is that a lab board, or a node that survives rain, theft, and 3 months without you? |
| [Condition monitoring of WSN weather stations](https://publications.eai.eu/index.php/IoT/article/view/661) | Moving-median filter on the ESP32 | How do you know the *sensor itself* is lying, not just filtering junk? |
| [Network densification for Uganda weather stations](https://doi.org/10.1109/istafrica.2016.7530633) | 69 KCCA blackspots | Where do you put the first 3 nodes, and why not 69 at once? |
| Rainy-season / night battery on WIMEA AWS | Solar + ESP32 sampling every 30 s | 30 s may kill the battery. She will ask for a power budget. |
| Requirements elicitation (she lists this on her [LinkedIn](https://linkedin.com/in/mary-nsabagwa-6139688a)) | “KCCA needs this” | Have you talked to a drainage engineer yet? (Honest answer: not yet. That is fine if you say so.) |

She also worked on AdEMNEA (bee / fruit-fly IoT) and supervised the 2023 “Eye for the Blind” team. Pattern: **community problem + student-buildable hardware + field reliability**. TabaPort fits that pattern if you do not oversell it.

## What to research before you knock on Room 301

Do this in order. Do not try to read 20 papers.

**1. Her 2019 AWS paper (1–2 hours)**  
Write one page of notes with these headings, in her language:

- Reliability  
- Data accuracy  
- Power consumption  
- Data transmission  
- Cost  

Under each, write what TabaPort currently *assumes* and what you have **not** tested. That page is your meeting ticket.

**2. The condition-monitoring paper (45 minutes)**  
She looks at arrival rate and packet content to tell whether a *sensor*, a *node*, or a whole *station* is sick. For floods this matters more than weather, because a dead node looks like “no flood.” Prepare one sentence:

> If a culvert node goes silent, TabaPort should treat that as “unknown,” not “safe.”

**3. Hardware you can defend, not just name**  
Before she asks, know:

- Sensor: JSN-SR04T (or A02YYUW) ultrasonic, ~UGX 25,000 at Neriko, Wandegeya. Range, 5 V, splash-proof probe, **not** meant to sit under dirty water full of plastic.  
- MCU: ESP32, ~UGX 60,000. Why ESP32 (Wi-Fi + BLE, cheap, local stock) vs Arduino / STM32.  
- Radio fallback: Wi-Fi in the lab; LoRa (~UGX 80,000) for the field; SMS only if the cell is up.  
- Sample every 30 s is a Phase 2 claim. Be ready to drop it to 1–5 minutes if she says the power budget is wrong.

**4. One honest demo plan**  
You are not asking her to design the IEEE video. You are asking her whether **one node in a water tank + laptop hub** is a sound first generation, the same way her 2019 paper was a first-generation AWS.

## Questions worth her time

Ask only what an embedded / WSN person can answer. Leave KCCA politics and the IEEE video script at home.

1. For a first-generation node, is waterproof ultrasonic acceptable in a **tank demo**, or should I start with a pressure sensor so the later field version is not a dead end?  
2. In your AWS work, what actually killed nodes in the field: power, radio, water ingress, or vandalism? Which of those should I design for first?  
3. Is 30-second sampling realistic on solar + a small Li-ion in Kampala’s rainy weeks, or should I sleep most of the time and wake on a rise threshold?  
4. For 3 nodes at Clock Tower / Bwaise / Kalerwe, would you use LoRa to a laptop hub, or GSM/SMS first because LoRa needs a gateway I do not have?  
5. How would you detect a fouled or drifted ultrasonic so the dashboard does not show green when the culvert is full?  
6. If I build one node this month, what would you want to see in a lab log before you would call it “working”?

That last question is gold. It gives her a way to supervise you without committing to KCCA.

## Questions that waste the meeting

- “Can you introduce me to KCCA?” Too early. Ask that after she has seen a node.  
- “Can you fund this?” No.  
- “Is my IEEE idea good?” She cannot judge the challenge. She can judge the hardware.  
- A 20-minute lecture on the Drainage Master Plan. One minute of context is enough.  
- Asking her to pick your entire BOM while you have bought nothing and written no firmware.

## How to approach her

Go as a **student with a Phase 3 deadline and a first-generation hardware question**, not as someone asking her to join the team.

Walk to Block A, Room 301 during office hours if you already know her from class. If you need a written trail, email first.

> **Subject:** Advice on a first-generation flood-sensor node (IEEE Phase 3)  
>  
> Dear Dr. Nsabagwa,  
>  
> I am Shakiran Nannyombi. I take / have taken your embedded systems work seriously, and I would like 20 minutes of advice before I buy hardware.  
>  
> My project, TabaPort, is an offline-first flood dispatch tool for Kampala. IEEE selected it for Phase 3 of the Response Quest Challenge. The submission is a 2–5 minute video of a real person using a working prototype, due 9 October. I am not approaching KCCA yet.  
>  
> I have read your 2019 paper on a robust, affordable automatic weather station. I want the first TabaPort node to be a first-generation prototype in the same sense: cheap, local, and honest about reliability, accuracy, power, and radio.  
>  
> Proposed first node: ESP32 + waterproof ultrasonic in a test tank, laptop as the hub, later LoRa/GSM. I am unsure whether ultrasonic is the right sensor once this leaves the lab, and whether 30-second sampling is realistic on a small solar pack.  
>  
> I can come to Block A, Room 301 at your convenience this week or next. I will bring a one-page note and a parts list, not a request for funding.  
>  
> Kind regards,  
> Shakiran Nannyombi  
> [programme, year] | IEEE member | TabaPort #5267  
> [phone]

Her public faculty page also lists a mobile number. Use email or the office first. Calling a lecturer’s phone before she has replied looks careless.

## What to put on the one-pager (one side of A4)

1. One sentence: TabaPort = culvert water level → offline hub → red/yellow/green roads → ranked KCCA jobs.  
2. IEEE Phase 3: working demo by 9 October; one physical node is enough.  
3. Your four-box table copied from her paper (reliability / accuracy / power / radio) with “tested” vs “not tested.”  
4. Proposed BOM and price (ESP32 + JSN-SR04T + box ≈ UGX 120,000).  
5. The six questions above.

Print two copies. Leave one.

## After she talks, then KCCA

Her advice will change the hardware story you take to Engineering at City Hall. If she says “ultrasonic will fail in a silted channel, use a radar or a stilling well,” you do not want to have already promised KCCA an ultrasonic network. That is why this order is right: **Nsabagwa → one lab node → IEEE video → KCCA**.

If you want, I can draft that one-pager next in the same style as `advice.md`, with the four-box table already filled from your Phase 2 PDF so you only have to correct what you have actually tested.