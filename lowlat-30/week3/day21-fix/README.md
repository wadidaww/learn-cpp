# Day 21 · FIX order entry, binary protocols, Mock 3

**Why trading firms care:** every firm talks FIX to some broker or venue, and many low-latency interviews ask you to parse it. It is also the cleanest example of "why text protocols are slow" and of the session rules (sequence numbers, resend) that keep order entry safe.

## 1. Concepts (45 min)

### The wire format
A FIX message is `tag=value` fields, each ended by **SOH** (byte `0x01`, shown as `|` in logs):
```
8=FIX.4.4|9=108|35=D|49=KRESNA|56=EXCH|34=42|52=20261127-14:30:00.123|11=ORD-0001|55=AAPL|54=1|38=100|40=2|44=189.0100|59=0|10=095|
```
| Tag | Name | Rule |
|---|---|---|
| 8 | BeginString | Always first. Protocol version |
| 9 | BodyLength | Always second. Number of bytes **from the byte after the SOH that ends 9=, up to and including the SOH before 10=** |
| 35 | MsgType | Always third (first field of the body). `D` NewOrderSingle, `F` cancel, `G` cancel/replace, `8` ExecutionReport |
| 49 / 56 | SenderCompID / TargetCompID | Who is talking to whom |
| 34 | MsgSeqNum | Per-direction sequence number, starts at 1 each session (or each day) |
| 52 | SendingTime | UTC |
| 10 | CheckSum | Always last. Sum of every byte before `10=`, mod 256, written as exactly 3 digits |

NewOrderSingle body: 11 ClOrdID (your id, unique per day), 55 Symbol, 54 Side (`1` buy, `2` sell), 38 OrderQty, 40 OrdType (`1` market, `2` limit), 44 Price, 59 TimeInForce (`0` day, `3` IOC, `4` FOK).

ExecutionReport (`35=8`) answers it: 150 ExecType and 39 OrdStatus (`0` new, `1` partial, `2` filled, `4` cancelled, `8` rejected), 14 CumQty, 151 LeavesQty, 31/32 LastPx/LastQty, 37 OrderID (the exchange's id).

### The session layer
- **Logon (A)** with HeartBtInt (108). **Heartbeat (0)** when idle; **TestRequest (1)** when the other side is silent, and disconnect if no answer.
- **Sequence numbers** detect loss. Received seq higher than expected: send **ResendRequest (2)**. Lower than expected without PossDupFlag (43=Y): fatal, logout. The other side replays the messages, or sends **SequenceReset-GapFill (4)** for admin messages that should not be replayed (you never want a stale order resent hours later).
- **Logout (5)**, **Reject (3)** for malformed messages (session level), BusinessMessageReject (j) for application level.
- Order entry is stateful: after a disconnect you reconcile every live order (OrderStatusRequest, or the venue's drop-copy) before you trade again. Many venues cancel all your orders on disconnect ("cancel on disconnect").

### Why FIX is slow, and what replaces it
- Text: every integer and price is converted to and from decimal. Fields are variable-length, so you cannot jump to an offset; you must scan for SOH and `=`.
- BodyLength comes before the body and CheckSum covers everything: an encoder must know the body size before writing it, and touch every byte again.
- Binary order entry protocols used by fast venues: **OUCH** (Nasdaq: fixed-size binary messages over SoupBinTCP, like ITCH for orders), **BOE** (Cboe), **SBE/iLink 3** (CME, Simple Binary Encoding over FIXP), **ETI** (Eurex). Fixed offsets, little or big endian integers, no checksum (TCP already has one).

### Fast FIX tricks (say these in the interview)
- **Precompute** the constant parts (8, 35, 49, 56, and for a given instrument 55) once; per order write only what changes.
- **Reserve space** for BodyLength and fill it last, or fix the length of numeric fields with zero padding where the counterparty accepts it, so the length is a constant.
- **Incremental checksum**: the sum of the constant part is precomputed; add only the bytes you wrote.
- Integer to text with a two-digits-at-a-time lookup table; prices as fixed point integers, never `double` (and never `printf`).
- Decode with a single pass over the bytes: read the tag as an integer while scanning, switch on it, and keep `string_view`s into the receive buffer instead of copying.
- No allocation, no exceptions on the hot path. Validate only what you must (the venue validates too).

## 2. Exercise (2 h)

In `solution.hpp`:
1. `checksum`: sum of bytes mod 256.
2. `encode_new_order`: write the whole message into a caller buffer with the exact tag order in the header comment, compute 9 and 10, return the length (0 if it does not fit or the order is invalid). Hint: compute the body length first (you know the size of every field), then write in one pass.
3. `decode_new_order`: check the framing (`8=FIX.4.4`, a numeric 9, `10=` exactly where 9 says, a correct checksum), then parse the body fields in any order, ignoring unknown tags. Return `incomplete` for any prefix of a valid message, so a TCP reader can wait for more bytes (like day 15).

The tests compare your encoder byte for byte with an independent framer, try every truncation, every framing error and bad field, and count heap allocations (they replace `operator new`). Then:
```bash
cmake --build build --target day21_test day21_bench && ./build/day21_test && ./build/day21_bench 2
```
Note your encode and decode p50 and compare them with yesterday's ITCH parser. Explain the difference in one sentence (text vs fixed-offset binary).

Stretch: make the encoder precompute everything up to and including `34=` once per session (a template), and measure again.

**Done when:** the tests pass under ASan+UBSan, and you can draw the session layer (logon, heartbeat, gap, resend, gap fill, logout) on a whiteboard.

## 3. Say it out loud

<details><summary>1. What exactly does BodyLength count, and what does CheckSum cover?</summary>
BodyLength (9) counts the bytes after the SOH that ends the 9= field, up to and including the SOH before 10=. CheckSum (10) is the sum of every byte from the start of 8= up to and including that SOH, mod 256, written as three digits.
</details>

<details><summary>2. You receive MsgSeqNum 105 but expected 101. What happens?</summary>
You process nothing past the gap as new, send a ResendRequest for 101 to 104 (or 101 to infinity), and the other side replays them with PossDupFlag=Y, or sends a SequenceReset-GapFill over admin messages and over orders it does not want to replay. Then you continue from 106.
</details>

<details><summary>3. And if you receive 99 when you expected 101?</summary>
A lower sequence number without PossDupFlag means the sessions are out of sync: it is a serious error. Send Logout with a reason and disconnect; operations investigate. With PossDupFlag=Y it is a duplicate replay and you ignore it.
</details>

<details><summary>4. Why is FIX slower than OUCH or SBE?</summary>
Text: numbers must be converted to and from decimal, fields are variable-length so you scan for delimiters instead of reading fixed offsets, and the length and checksum need extra passes. Binary protocols are fixed layouts you read with a load at a known offset.
</details>

<details><summary>5. How do you make a FIX encoder fast?</summary>
Precompute the constant prefix and its checksum, write only the changing fields with table-driven integer formatting, keep prices as fixed point integers, compute the body length up front (or fix field widths), update the checksum incrementally, no allocation, write straight into the send buffer.
</details>

<details><summary>6. What is ClOrdID vs OrderID?</summary>
ClOrdID (11) is yours, unique per trading day, and you use it to cancel or replace (OrigClOrdID 41 refers to the previous one). OrderID (37) is assigned by the venue and comes back in ExecutionReports.
</details>

<details><summary>7. Your FIX session drops while you have 50 live orders. What do you do?</summary>
Stop sending new orders. Reconnect and log on (the venue replays what you missed through the sequence number logic), reconcile every order through ExecutionReports or OrderStatusRequest or drop-copy, and only then resume. Know whether the venue cancels on disconnect. Risk limits assume the worst case (all orders filled) until reconciled.
</details>

## 4. Mock 3 (in the thread: say "start mock 3")

60 minutes: 15 minutes of rapid questions, 30 minutes writing a parser (ITCH or FIX, the interviewer picks) from a blank file, 15 minutes on networking and host tuning follow-ups.

**Networking**
1. Walk through what happens between `send()` on one host and `recv()` returning on another.
2. What is Nagle's algorithm, why does it hurt latency, and what is delayed ACK's role in the 40 ms stall?
3. TCP is a byte stream. How do you frame messages? What must the reader do with a partial message?
4. Level-triggered vs edge-triggered epoll: what changes in your read loop?
5. Busy polling vs blocking: what do you gain, what does it cost?
6. Why do market data feeds use UDP multicast? What do you lose compared with TCP?
7. How do A/B feed lines work, and how do you arbitrate them?
8. You detect a sequence gap on both lines. What are your options?
9. What does kernel bypass remove from the path? Name two ways to do it.
10. What is a hardware timestamp, and why would you want one?

**Host and OS**
11. List the tuning steps for a low-latency Linux host, with one reason each.
12. What do isolcpus, nohz_full and rcu_nocbs each do?
13. Why do C-states and frequency scaling matter for a thread that never sleeps?
14. What is an SMI and how would you detect one?
15. Why does NUMA placement matter for the NIC and the trading thread?
16. How do you find out what interrupted your core?

**Market microstructure and protocols**
17. How does a market maker earn money, and how does it lose it?
18. What is adverse selection? Give an example.
19. Why is queue position valuable?
20. Market-by-order vs market-by-price: what does each send?
21. What does an ITCH Order Replace do to the book?
22. An ITCH execution arrives for an order reference you do not know. What do you do and what might it mean?
23. What do BodyLength and CheckSum cover in FIX?
24. How does FIX recover from a sequence gap?
25. Why are binary order entry protocols faster than FIX?
26. What is cancel-on-disconnect, and why do venues offer it?

**Design follow-ups**
27. How would you structure a feed handler for 8,000 symbols on one core?
28. Your book is wrong after a gap. How do you recover (snapshot, replay, or both)?
29. How do you test a parser? (Recorded data, byte-by-byte truncation, fuzzing, a differential model.)
30. Where would you put timestamps to measure tick-to-trade?

Week 3 checklist before moving on:
- [ ] TCP framer and ITCH parser from memory
- [ ] The host tuning checklist recited with reasons
- [ ] Your jitter table (day 19) and your ITCH and FIX parse numbers, with the machine described
- [ ] Weak list from week 3 re-answered without notes
