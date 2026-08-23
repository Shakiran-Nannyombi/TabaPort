TabaPort is still something KCCA needs. Drainage construction does not replace it. Construction moves water. Your system tells responders **where the water is, which roads are dead, and where to send a crew**, including while those channels are still being built.

That is the line to hold with KCCA and in the Phase 3 video.

## Why this still matters after the bridges and channels

KCCA’s own public position is that engineering works have helped Bwaise and parts of downtown, but the city is **not** “solved.” From their July 2026 Parliament briefing and August flood-preparedness updates:

- The drainage network is **362.3 km** across eight catchments (Nakivubo, Lubigi, Walufumbe, Nalukolongo, Kansanga, Kinawataka, Nakamiro, Mayanja).
- They have identified **69 flood blackspots** that still need active monitoring.
- Lubigi still has **980 m** waiting to be finished. Nsoba and Kiwunya works have only just started. Jugula (between St. Balikuddembe Market and Nakivubo Stadium) is **more than 90% encroached**.
- They say they need **UGX 768 billion from 2026/27 to 2030/31**. That is a five-year civil-works programme, not a finished city.
- When rains start, KCCA will **pre-position trucks and emergency teams**. Those teams still have to choose a street. Right now that is mostly radio, WhatsApp, and people standing in water.
- Parliament has already put KCCA on a short leash over flooding, including a [7-day report demand](https://www.monitor.co.ug/uganda/news/national/parliament-gives-kcca-7-days-to-explain-kampala-flooding-5543126) after inspections at Nalukolongo–Najjanankumbi, Lugogo, and Kyambogo.
- UNDP’s figure, cited in that inquiry: about **US$50 million / UGX 188 billion** a year in flood damage.

KCCA Deputy ED Benon Kigenyi said it plainly: *“We cannot stop the rain, but we can prepare Kampala to manage it better.”* He also said drainage works fail if waste and silt keep blocking channels. That is in [KCCA’s own update](https://www.kcca.go.ug/news/1154/) and the [August El Niño preparedness story](https://ubc.go.ug/2026/08/12/kcca-steps-up-drainage-works-to-tackle-flooding-in-kampala/).

So TabaPort is not competing with Nakivubo rehabilitation or the new culverts on Allen and Ssebana roads. It sits **on top** of that work:

| KCCA is building | TabaPort adds |
|---|---|
| Bigger channels and culverts | Live water-level at those same structures |
| 69 blackspots to watch by patrol | Ranked dispatch list so the first truck goes to the worst site |
| Trucks staged when rain starts | Offline map of which roads are red / yellow / green |
| Waste and silt as the remaining failure mode | Sensor filter that ignores floating plastic, plus a USSD report from a boda who saw the blockage |
| 5-year master plan | Eyes during the years of construction, when diversions and incomplete links often **worsen** local flooding |

The [NTV piece](https://ntv.co.ug/news/kcca-urges-land-donation-to-solve-kampala-flooding) is useful here too: KCCA says some expansions cannot happen until people give land. That means several corridors will stay undersized for years. Those are exactly the sites that need sensors and rerouting.

The [Monitor swimming editorial](https://www.monitor.co.ug/uganda/oped/editorial/survival-swimming-is-a-must-learn-for-all-5025982) is a different problem (drowning). Use it only as context: floods kill in unexpected places, including streets. Do **not** pitch TabaPort as swim training. Pitch it as keeping responders and the public **off submerged roads**.

## Be honest about reliability before you walk into City Hall

KCCA Engineering will respect a student who knows the limits. They will dismiss a pitch that overclaims.

**What you can defend today**

- Offline-first hub (laptop / Raspberry Pi + local map) when MTN/Airtel congest or drop.
- Colour-coded roads: green / yellow / red.
- AHP-style ranking: rise rate + near a hospital/flyover + dense neighbourhood.
- A* reroute that applies an infinite penalty to a flooded segment (Clock Tower → Northern Bypass, etc.).
- A moving-median filter on the ESP32 so a plastic bottle does not look like a flash flood.
- Privacy: strip phone numbers at the SMS/USSD doorway.

**What you should not promise KCCA or IEEE as if it already works**

- “Zero-cost SMS to every phone in a 2 km radius.” That is **cell broadcast**. Only UCC plus the telcos can do it. A Raspberry Pi cannot. For Phase 3, show a **draft alert on the operator screen** and say partnership with MTN/Airtel/UCC is the production path.
- City-wide accuracy of ultrasonics in dirty, debris-filled culverts. The filter is the right idea. You still need a 30-day field test before anyone should trust it for dispatch.
- Installing nodes inside public drains without written KCCA permission. That is their asset. Ask first.
- That TabaPort “stops flooding.” It does not. It reduces blind deployment and people driving into water.

If they ask “how reliable is this?”, answer like this:

> For the IEEE Phase 3 demo, reliability means the interface updates in seconds, ranking and routing stay available with no internet, and the sensor ignores junk in a test tank. For KCCA operations, reliability means a 30-day trial on 2–3 of your 69 blackspots, side-by-side with your existing patrol reports, before any scale-up.

That sentence will get you a second meeting.

## Money: three different budgets

You do **not** need a city network to submit Phase 3. You need a working product a real person can use on camera by **9 October**. IEEE forbids AI-generated, simulated-only, or stock footage. A person tapping a live dashboard is enough. One physical sensor in a bucket of water makes it much stronger.

Prices below use current Kampala shop ranges (for example [Neriko Electronics, Wandegeya](https://www.nerikoelectronics.com/): ESP32 about UGX 60,000, JSN-SR04T about UGX 25,000, LoRa module about UGX 80,000, Raspberry Pi 4 4GB about UGX 620,000). Treat them as planning figures, not invoices.

### Budget A — Phase 3 video (what you need now)

**About UGX 150,000–450,000 (roughly US$40–120).** You can do the low end with only a laptop.

| Item | Low | Better |
|---|---|---|
| Software dashboard on your laptop | UGX 0 | UGX 0 |
| ESP32 + waterproof ultrasonic + wires + box | — | ~UGX 120,000 |
| Phone power bank / small solar for the node | — | ~UGX 40,000–80,000 |
| Video: phone + friend as “KCCA dispatcher” | UGX 0 | UGX 50,000 transport/airtime |
| Optional Raspberry Pi hub | skip | ~UGX 620,000 |

**Recommendation:** spend **UGX 200,000–300,000**, build the web app first, add **one** sensor in a clear water tank for the video. Skip the Pi if money is tight. A laptop *is* the edge hub for Phase 3.

### Budget B — KCCA 30-day pilot (ask for this later, not now)

**About UGX 4–8 million (roughly US$1,100–2,200)** for 8–12 nodes on the worst of the 69 blackspots, plus one field laptop, SIM airtime, enclosures, and a week of your time on site.

That is the ask *after* they have seen the demo. Do not ask for this in the first email.

### Budget C — Division-scale (only if a grant or IEEE prize lands)

**About UGX 40–80 million** for one division (~40–60 nodes, 2 hubs, rugged boxes, solar, vandal-resistant mounts, 6 months maintenance). Still tiny next to UGX 768 billion of civil works. That is how you should frame cost to KCCA: **an operations layer, not a competing infrastructure project.**

Your Phase 2 note of “under US$55 per node” is still the right target for a production node. A first handmade node in Kampala will land closer to **UGX 250,000–350,000** once you add solar, battery, box, and LoRa.

### Where the Phase 3 money can come from this week

Do not wait for KCCA. They move slowly, and the video is due 9 October.

1. Personal / family / IEEE student branch (UGX 200k–300k).
2. Makerere CEDAT / IEEE Uganda Section lab kit or a weekend bench.
3. Later, not for the video: IEEE HAC / SIGHT, Innovation Village, UNDP/UNCDF urban-resilience windows, MTN/Airtel CSR. Those are 2027 conversations.

## How to approach KCCA so you look prepared

Do not start with the Executive Director or with “please fund my project.” Start with **Engineering and Technical Services**, which already owns drainage and the 69 blackspots. The NTV story names **Justus Akankwasa** as Director of Engineering and Technical Services. Deputy ED **Benon Kigenyi** is the public face of the flood file. Spokesperson **Daniel Nuweabiine** is the protocol door. General contact: [info@kcca.go.ug](mailto:info@kcca.go.ug), City Hall, Plot 1–3 Apollo Kaggwa Road, tel 0417 900 100 / toll-free 0800 199 000.

A Makerere or **IEEE Uganda Section** introduction is better than a cold email.

Ask for **20 minutes and a reaction**, not money. You want three facts only they have:

1. How do field engineers get a flood report today (radio, WhatsApp, walk-in)?
2. Which 5 of the 69 blackspots should a student instrument first?
3. Would they allow a **non-invasive** demo (laptop + one sensor on KCCA premises, not inside a live channel) during the September–December El Niño window?

### Email you can send

> **Subject:** Student prototype to support KCCA flood dispatch at the 69 blackspots — request for a 20-minute technical briefing  
>  
> Dear Directorate of Engineering and Technical Services,  
>  
> My name is Shakiran Nannyombi. I am an IEEE member. My project, TabaPort, was selected for Phase 3 of the IEEE Response Quest Challenge (a global emergency-intelligence competition).  
>  
> TabaPort does not replace drainage construction. It is an offline-first dispatch tool for the years while channels are still being built, and after they are built, when waste and silt still block them. It is meant to help the crews you already pre-position when rain starts.  
>  
> I have read KCCA’s July 2026 Parliament briefing and the August El Niño preparedness update: 69 blackspots, 980 m of Lubigi still outstanding, Jugula heavily encroached, and UGX 768 billion needed through 2030/31. Those points are why I am building this.  
>  
> I am not asking for funding. I am asking for 20 minutes with an engineer who dispatches drainage or emergency teams, so I do not design against the wrong workflow. I can bring a working laptop demo.  
>  
> Three questions I hope to check: how a flood report reaches a crew today; which blackspots are highest priority for a small sensor trial; and whether a non-invasive demo on KCCA premises would be acceptable.  
>  
> I can come to City Hall at your convenience before 30 September.  
>  
> Kind regards,  
> Shakiran Nannyombi  
> IEEE member | TabaPort (#5267)  
> [phone] | [email]

Take to the meeting: the laptop demo, a one-page diagram (sensor → hub → red/yellow/green map → ranked jobs → reroute), and the sentence on reliability above. Leave Budget B as a printed annex. Do not lead with it.

## What to build for the 2–5 minute IEEE video

Deadline is **9 October**. Today is 23 August. You have about seven weeks. The 5 August IEEE check-in has already passed; send a short status email this week anyway so they know you are building.

A person must be on camera using the product. Film a friend as a KCCA dispatcher at a table. Phone camera is fine. No stock clips, no AI voice, no fake “click this and the city updates” animation unless the software actually updates.

**Shot list (about 3.5 minutes)**

1. **15s** You, on camera: “Kampala flash floods close Clock Tower, Bwaise, Kalerwe in minutes. When the network dies, KCCA field teams go blind. This is TabaPort.”
2. **30s** Hands pour water into a tank. The ESP32 reading climbs. Dashboard goes yellow, then red.
3. **45s** Dispatcher opens the map. Clock Tower / Nakivubo segment turns red. Priority list reorders (hospital and dense market rise).
4. **45s** They tap “Navigate.” A* drops the flooded road and offers a high-ground alternative.
5. **30s** They confirm a USSD-style boda report (“Kalerwe blocked”). Hub parses it and draws a 100 m uncertainty circle until a sensor confirms.
6. **20s** Operator preview of the public SMS (clearly labelled as operator-triggered, not a live city blast).
7. **20s** You: “This runs on the laptop with no cloud. Same idea scales to KCCA’s 69 blackspots while drainage works continue.”

That hits IEEE’s three tests: real person, core function, real-time response, Kampala scenario.
