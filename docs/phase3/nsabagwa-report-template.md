# Template: technical note for Dr. Mary Nsabagwa

This is a **shell**. You write every section after you have read her papers. Do not send this file as-is.

**Audience:** one lecturer who already knows WSN, power, and outdoor sensors.  
**Aim:** she can read it alone in 10–15 minutes, then tell you whether the first-generation node is sound.  
**Length:** 4–6 pages, or about 1,200–1,800 words. If it is longer, cut the KCCA story, not the hardware.  
**Tone:** student technical note. Short sentences. No sales language. No “please fund this.” No IEEE prize talk beyond one line on the deadline.  
**Title on the cover:** something like *TabaPort: a first-generation flood-level node — request for technical comment*.

Print it as PDF. Attach it to the email. Leave a paper copy at Room 301 if you go in person.

---

## How to fill this

1. Read her 2019 AWS paper. For each of her four non-functionals (reliability, accuracy, power, transmission), write what TabaPort *claims* and what you have *not* tested.
2. Read the 2018 condition-monitoring paper. Add one paragraph on what happens when a node goes silent.
3. Fill sections 1–8 below in your own words. Delete every prompt in square brackets.
4. Keep the questions in section 7. Those are why she is reading it.
5. Add a references list. Cite her work first.

**Do not put in this report**

- How to “manage” the meeting
- A Drainage Master Plan essay
- Cell-broadcast SMS to every phone in 2 km (you cannot do this)
- A request for money or a KCCA introduction
- Unmeasured numbers presented as results

---

## Cover (half a page)

**Title:** [your title]

**Author:** Shakiran Nannyombi  
**Programme / year:** [e.g. BSc Computer Science, Year …]  
**Contact:** [email] · [phone]  
**Date:** [day month 2026]  
**For:** Dr. Mary Nsabagwa, Department of Networks, Makerere CoCIS  
**Purpose:** comment on the first-generation sensor node before hardware is purchased  
**Deadline I am working to:** IEEE Response Quest Phase 3 video, 9 October 2026

**One sentence (the whole project):**  
[Culvert water level → offline hub → red/yellow/green roads → ranked jobs for a dispatcher.]

**One sentence (what I want from you):**  
[I want to know if this first node is a sound generation-1 design, in the same sense as a first-generation AWS.]

---

## 1. Problem I am targeting (half a page)

[3–5 sentences. Kampala flash floods. Responders cannot see which culvert is overflowing when the network is congested. I have **not** yet spoken to KCCA. This note is only about the sensing node.]

[One sentence on why construction of channels does not remove the need for a level sensor.]

---

## 2. Proposed system (one page)

Draw a simple box diagram (hand-drawn is fine, then photograph it):

`sensor node → radio → laptop hub → map (green / yellow / red) → ranked list → reroute`

Then write four short subsections. Stay at generation 1. Mark later ideas as **later**.

### 2.1 Sensor node (generation 1 — this is what I am asking about)

- MCU: [ESP32, and why]
- Sensor: [JSN-SR04T or other, and why]
- Sample interval: [30 s is the Phase 2 claim; say if you now doubt it]
- Filter: [moving median — window size if you have one, else “not yet chosen”]
- Power: [USB / power bank for the lab; solar is later]
- Enclosure: [bench box, not a live culvert]

### 2.2 Hub (generation 1)

[Laptop. Offline. No Raspberry Pi required for Phase 3.]

### 2.3 What the dispatcher sees

[Green / yellow / red. If the node is silent, the state is **unknown**, not green.]

### 2.4 Out of scope for this note (later)

[LoRa city network, public SMS blast, KCCA install, USSD from bodas. List them so she knows you are not asking her to review the whole IEEE concept.]

---

## 3. What I took from the literature (one page)

Write this **after** you read. Use her headings. Three to five citations is enough.

### 3.1 Nsabagwa et al. (2019) — first-generation AWS

[In your words: they built a first generation, then said the next one had to improve reliability, accuracy, power, and transmission. Cost comes after those four.]

[Then: I am treating TabaPort the same way. Generation 1 is a tank + laptop, not a city network.]

### 3.2 Nsabagwa et al. (2018) — condition monitoring

[In your words: a quiet or lying node is a failure mode. For floods, silence must not look like “safe.”]

[How I will apply it: unknown state; later, check arrival rate / packet sanity.]

### 3.3 One extra source (optional)

[Densification 2016, or a sensor datasheet, or a flood-sensing paper. Only if you actually read it.]

---

## 4. Gap analysis — her four non-functionals (one page)

This is the section she will mark up. Be blunt.

| Non-functional | What I claimed in Phase 2 | What I have tested | What I will test in the lab | What I am unsure about |
|---|---|---|---|---|
| Reliability | | Nothing yet | | |
| Data accuracy | | Nothing yet | | |
| Power consumption | | Nothing yet | | |
| Data transmission | | Nothing yet | | |

Under the table, one short paragraph per row. No excuses.

**Cost (only after the four):**  
[Phase 2 said under US$55. Generation-1 kit I can buy in Kampala is about UGX … . I will not buy a solar/LoRa kit until this note is commented.]

---

## 5. Proposed generation-1 test (half a page)

Write a test she can picture.

1. [Mount probe above a clear tank. Measure empty distance as baseline.]
2. [Add water in steps. Log raw and filtered level every N seconds.]
3. [Float bottles or a bag. Show whether the median holds.]
4. [Unplug Wi-Fi or USB. Dashboard must go to unknown, not green.]
5. [If I can: measure current awake vs sleep.]

**What I will put in a lab log:** date, firmware version, window size, raw vs filtered CSV, one photo of the tank, pass/fail on unknown-state.

**What I will not do yet:** install anything in a public drain.

---

## 6. Risks I already see (half a page)

Bullet list. You may add more after reading.

- Ultrasonic + plastic debris → false spikes or lost echo
- 30 s sampling → battery death in a rainy week
- Humidity / condensation on the probe
- Theft or splash if this ever leaves the lab
- Silent node misread as “no flood”
- I have not elicited requirements from a KCCA drainage engineer

---

## 7. Questions I am asking you to mark (keep these)

Number them. Leave space on the printed page for her pen.

1. For generation 1, is waterproof ultrasonic acceptable in a **tank**, or should I start with a pressure (or other) sensor so the field version is not a dead end?
2. In field AWS work, what actually killed nodes first: power, radio, water ingress, or vandalism? Which of those should I design for first?
3. Is 30-second sampling realistic on a small pack in Kampala’s rainy weeks, or should the node sleep and wake on a rise threshold?
4. If I later try three sites (Clock Tower, Bwaise, Kalerwe), would you start with LoRa to a laptop hub, or GSM/SMS, given I do not have a LoRa gateway?
5. How should I detect a fouled or drifted ultrasonic so the map does not show green when the channel is full?
6. If I build one node this month, what would you want in the lab log before you would call it working?

---

## 8. What I am not asking (four lines)

I am not asking for funding.  
I am not asking for an introduction to KCCA in this first note.  
I am not asking you to join the IEEE team.  
I am asking whether this generation-1 node is technically reasonable to build next.

---

## References

Use a simple numbered list. Put her papers first.

1. Nsabagwa, M., Byamukama, M., Kondela, E., & Otim, J. S. (2019). Towards a robust and affordable Automatic Weather Station. *Development Engineering*, 4, 100040. https://doi.org/10.1016/j.deveng.2018.100040
2. Nsabagwa, M., Sansa Otim, J., Nyongarwizi Akol, R., et al. (2018). Condition Monitoring for Wireless Sensor Network-Based Automatic Weather Stations. *EAI Endorsed Transactions on Internet of Things*, 4(14), e4. https://publications.eai.eu/index.php/IoT/article/view/661
3. [Add only what you read: 2016 densification paper, JSN-SR04T datasheet, KCCA news item, Phase 2 submission.]

---

## Before you send — checklist

- [ ] I read the 2019 paper and wrote section 3.1 in my own words
- [ ] I read the 2018 paper and wrote section 3.2 in my own words
- [ ] Section 4 table is honest (nothing marked “tested” that I did not run)
- [ ] Generation 1 and “later” are clearly separated
- [ ] Questions in section 7 are still there
- [ ] Cover has my name, programme, email, date
- [ ] File is a PDF, 4–6 pages, readable if she prints in black and white
- [ ] Email is short: one paragraph + “the note is attached” + offer to come to Room 301

**Email subject:** Technical note on a first-generation flood-level node — request for comment

**Email body (keep it shorter than the report):**  
who you are · IEEE Phase 3 due 9 October · you wrote a short note · you have not bought hardware yet · you are not asking for funding · you can come to Block A 301 after she has read it.
