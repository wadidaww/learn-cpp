# Day 24 · Behavioural rounds, project stories, the 60-second intro

**Why trading firms care:** they hire people they trust with production systems that lose money in milliseconds when wrong. The behavioural round checks ownership, honesty about mistakes, how you handle pressure and disagreement, and whether you actually did what your resume says. A vague answer fails here even after a strong technical round.

## 1. The format (15 min)

Every story is **context, problem, what you did, result, what you learned**, in about 2 minutes:
- **Context** (2 sentences): team, system, why it mattered.
- **Problem** (2 sentences): what was wrong, with a number if you have one.
- **What you did** (most of the time): your decisions, the alternatives you rejected and why. Say "I", not "we", for your part.
- **Result** (1 sentence): a measured number, and how it was measured.
- **Learned** (1 sentence): what you would do differently.

Rules: every number must be one you can explain (how measured, on what, compared with what). If you do not remember a number, say the order of magnitude and that you do not remember the exact figure. Never inflate: the follow-up questions are designed to find it.

## 2. Write your six stories (90 min)

Write each one in your own notes, then say it out loud, record it, and cut it to 2 minutes. The brackets are for your facts.

**1. The compiler pass, O(n⁴) to O(n)**
- Context: [which compiler, which pass, what it did, who depended on it].
- Problem: [why it was quartic; what input size made it hurt; compile time on that input].
- What you did: [the insight that made it linear; the data structure; how you proved it equivalent: tests, differential testing on a corpus, review].
- Result: [compile time before and after, on what input, how measured].
- Follow-ups to prepare: Why was it quartic in the first place? How did you know the new one was correct? What did you not optimise and why?

**2. The 25% kernel speed-up**
- Context: [which kernel, which hardware].
- Problem: [where the time went; how you found it: profiler, counters].
- What you did: [the change, framed as data movement: "the time was in moving data between cores / through the memory hierarchy, so I ..."].
- Result: [25%: of what metric, on what workload, median of how many runs].
- The sentence to say: "Moving data between cores is the same problem as cache-line traffic in a trading system: the fastest code is the code that does not move the line."

**3. Leading five engineers**
- Context: [the project, the deadline].
- Problem: [scope, conflict or disagreement].
- What you did: [how you split the work, how you resolved the disagreement, one decision you got wrong and how you found out].
- Result: [delivered what, when].

**4. Your own order book (from week 2)**
- Design in three sentences, the number you measured with the machine, one alternative you rejected and why. Use your day 14 README.

**5. A mistake or failure**
- A real one: a bug you shipped, an estimate you missed, a wrong technical call. What happened, what you did in the first hour, what you changed afterwards so it could not happen again. Interviewers in trading care most about the first hour and the follow-up.

**6. A disagreement with someone senior**
- What you thought, what they thought, how you tested who was right (data, a prototype, a benchmark), what happened, and how the relationship was afterwards.

## 3. The 60-second intro (30 min)

Structure: who you are now, the one result that proves you are strong, why trading, why this firm. Template:

> I'm a [compiler / systems] engineer at [Huawei], where I've spent [two years] on [area]. The work I'm proudest of is [story 1 in one sentence, with the number]. Competitive programming at [ICPC level] is where I learned to [think in complexity and write correct code fast]. Over the last month I built [an L3 order book, an SPSC ring, an ITCH feed handler] from scratch and measured them: [one number, with the machine]. I want to move to trading because I like problems where nanoseconds are measurable and matter: in compilers I optimise for throughput, in trading the target is tail latency, and I want that feedback loop. [One sentence on this firm specifically.]

Record it. Play it back. Cut every filler word. It must be under 60 seconds and sound like you, not like a script.

## 4. Question bank (say each answer in under 2 minutes)

<details><summary>Why trading? Why now?</summary>Use story 5 from the roadmap: measurable nanoseconds, tail latency, a tight feedback loop. "Now" because you have built the fundamentals and want production systems where they decide outcomes.</details>

<details><summary>Why are you leaving Huawei after two years?</summary>Toward something, not away: you want latency-critical systems where performance is the product. Never criticise the current employer.</details>

<details><summary>Why this firm?</summary>One specific fact per firm: Jane Street (their engineering culture, OCaml and correctness focus, the HK office), Citadel Securities (market-making scale, low-latency team), Optiver or IMC (market making in Asia-Pacific, the trader-engineer collaboration). Read their engineering blogs or talks and name one.</details>

<details><summary>Tell me about a time you made a mistake in production.</summary>Story 5. Own it in the first sentence. Spend most of the time on detection, the first hour, and the fix that prevents it from happening again.</details>

<details><summary>Tell me about a disagreement.</summary>Story 6. Show that you settled it with data, and that the relationship survived.</details>

<details><summary>Describe the hardest bug you've fixed.</summary>Pick one where the cause was surprising (memory ordering, undefined behaviour, a compiler bug, a measurement artefact). Show the method: reproduce, narrow, hypothesis, test, fix, regression test.</details>

<details><summary>How do you handle pressure, e.g. something breaks during market hours?</summary>Stop the damage first (kill switch, roll back), then communicate, then diagnose. Never debug live while still trading. Tie to the kill-switch design from day 22.</details>

<details><summary>What do you do when you don't know the answer in an interview?</summary>Say what you do know, reason from first principles out loud, say how you would find out. Never bluff.</details>

<details><summary>What's a technical opinion you hold that others disagree with?</summary>Have one ready that you can defend with evidence, e.g. "measure p99.9, not the mean", "most lock-free code should be a lock", "virtual calls are fine off the hot path".</details>

<details><summary>Where do you see yourself in a few years?</summary>Owning a latency-critical system end to end, and trusted by traders to change it quickly and safely.</details>

<details><summary>Do you have questions for us?</summary>Always ask two: "What does the first six months look like for someone in this role?" and "How do you measure latency in production, and what changed the most last year?" The second shows you care about what they care about.</details>

**Done when:** you have six written stories, each said out loud in 2 minutes with every number defensible, and a recorded intro under 60 seconds.
