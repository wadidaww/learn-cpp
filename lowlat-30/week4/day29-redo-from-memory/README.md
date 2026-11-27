# Day 29 · Redo every build task from memory, timed

Interviews give you a blank file and a timer. Today you prove you can rebuild the core pieces without looking.

## How

`drill.sh` gives you a fresh copy of a day's original exercise (declarations only, as committed) plus its tests, in `drills/` (ignored by git), and compiles it with the sanitizers when you are done:
```bash
week4/day29-redo-from-memory/drill.sh day05          # creates drills/day05-<time>/, prints the build command
# ... implement solution.hpp there with a timer running, then:
week4/day29-redo-from-memory/drill.sh day05 check     # builds the newest drill with ASan+UBSan and runs the tests
```
Rules: no notes, no looking at your earlier solution, no autocomplete AI. Start the timer when you open the file; stop it when the tests pass under the sanitizers.

## Targets

| Task | Day | Target time |
|---|---|---|
| SPSC ring | 5 | 15 min |
| Seqlock | 6 | 15 min |
| Object pool | 10 | 20 min |
| Order book with matching | 11 | 45 min |
| Latency histogram | 12 | 30 min |
| Frame decoder | 15 | 20 min |
| ITCH parser and book | 20 | 45 min |
| FIX encode/decode | 21 | 40 min |
| Risk gate | 22 | 25 min |

Log every time in your notes. Anything over target: do it again tonight, then once more before your first onsite. Anything you could not finish goes to the top of the weak list with the reason.

## Then (1 h)
Explain each one out loud as if to an interviewer, in 3 minutes: the interface, the data layout, the invariant, the complexity, the measured number and the machine, the one thing you would improve.
