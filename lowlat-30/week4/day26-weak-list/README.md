# Day 26 · Fix the weak list, Jane Street-style coding

## 1. The weak list (2 h)

Open the weak list you have kept since day 1 (every question you missed or answered slowly, every bug a sanitizer found in your code, every mock comment).

1. Sort it into three groups: **don't know** (you could not explain it), **slow** (you got there, but took more than a minute), **sloppy** (you knew it but made a mistake).
2. **Don't know:** reread that day's README section 1, then write a 5-line explanation from memory and check it. Say it out loud.
3. **Slow:** answer it out loud three times in a row, each time faster, until it is under 45 seconds.
4. **Sloppy:** write the code again from a blank file, with the tests, under a timer (day 29 has the drill script).

Anything still on the list tonight gets a slot on day 29.

## 2. Jane Street-style coding (1 h 30)

Jane Street's software engineering interviews are less about C++ trivia and more about clean, correct code and clear thinking. Other firms' general coding rounds are similar. What they look for:
- You **clarify** the problem and agree on the interface before typing.
- The first version is **simple and correct**; you improve it only when asked or after it works.
- Your code reads well: small functions, good names, no clever tricks without a reason, invariants stated.
- You **test as you go**, with small and edge cases, out loud.
- You **change direction** gracefully when the interviewer adds a requirement (they always do).
- Curiosity about OCaml is a plus but not required: knowing that they value types that make illegal states unrepresentable is enough to talk about it.

Practise these four, 25 minutes each, in plain C++ (or Python if that is your interview language), out loud, then read your code back as a reviewer would:
1. **Order book replay:** given a list of add/cancel/execute events, output the best bid and ask after each event. Then: the events can arrive out of order by a bounded amount.
2. **Expression evaluator:** parse and evaluate `+ - * / ( )` with integers. Then: add variables and assignment.
3. **Rate-limited log:** print a message only if the same message has not been printed in the last 10 seconds. Then: memory must stay bounded.
4. **Interval merge for trading hours:** merge overlapping sessions per venue, then answer "is venue X open at time t" fast.

For each, write down afterwards: the interface you chose, the invariant, the test cases, and the follow-up change. That page is what you would say in the interview.

**Done when:** the weak list is shorter than it was this morning, and you have the four write-ups.
