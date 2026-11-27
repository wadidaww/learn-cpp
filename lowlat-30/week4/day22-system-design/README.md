# Day 22 · System design: a trading system end to end

**Why trading firms care:** almost every loop has a "design a trading system" or "design a feed handler / order gateway" round. They want boxes with clear ownership, the hot path separated from everything else, a latency budget you can defend, and failure modes you thought about before they ask.

## 1. The reference design (60 min: draw it from memory three times)

```
             exchange A feed (multicast A/B)        exchange B feed
                      │                                   │
              ┌───────▼────────┐                  ┌───────▼────────┐
  core 2      │ feed handler A │                  │ feed handler B │   core 3
              │ arbitrate, gap │                  │                │
              │ detect, decode │                  │                │
              └───────┬────────┘                  └───────┬────────┘
                      │ SPSC ring (normalised events)     │
              ┌───────▼───────────────────────────────────▼───────┐
  core 4      │ book builder → signals → strategy → RISK GATE      │   one thread, no locks,
              │ (per instrument books, top-of-book seqlock out)    │   event time only
              └───────┬────────────────────────────────▲──────────┘
                      │ SPSC ring (order intents)      │ SPSC ring (exec reports)
              ┌───────▼────────────────────────────────┴──────────┐
  core 5      │ order gateway: OUCH/FIX encode, session, seq nums, │
              │ throttle, cancel-on-disconnect handling             │
              └───────┬───────────────────────────────────────────┘
                      ▼  TCP to the venue
  housekeeping cores (0-1): logging thread (reads SPSC log rings), config, monitoring,
  drop-copy reconciliation, PnL, kill switch UI, recorder (pcap of every packet in and out)
```

Say each box in one sentence:
- **Feed handler:** receives both lines, keeps the first copy of each sequence number, detects gaps, recovers (retransmit request or snapshot), decodes into a small fixed-size event. Week 3, days 15 to 20.
- **Book builder:** applies events to an L3 or L2 book per instrument, publishes top of book. Week 2, day 11; week 3, day 20.
- **Strategy:** turns book state into order intents (quote, cancel, take). Static dispatch (CRTP or variant), no allocation. Week 1, day 3.
- **Risk gate:** synchronous checks on every intent before it can leave the box. Today's exercise.
- **Order gateway:** owns the session with the venue: encodes, sequence numbers, heartbeats, rate limits, reconnects. Week 3, day 21.
- **Everything else** runs on other cores and talks to the hot path only through SPSC rings or seqlocks, so it can never block it.

### Principles to say out loud
1. **One writer per piece of state.** The core thread owns books, positions and open orders. No locks; other threads read snapshots (seqlock) or receive copies (ring).
2. **Hot path does no I/O except its own sockets.** Logging is a binary record pushed into a ring; a housekeeping thread formats and writes it.
3. **Pre-allocate everything at startup.** Pools, rings, books sized from config. Allocation after start is a bug.
4. **Event time, not wall clock, inside the core.** Then the core is a pure function of its inputs: record the inputs and you can replay any day bit for bit, which is how you debug production and test strategy changes.
5. **Fail closed.** Unknown state (gap, disconnect, stale book, unreconciled orders) means stop quoting and cancel, not guess.

### Latency budget (how to talk about it)
Give orders of magnitude and say how you would measure each one. The ranges below are rough public figures that vary a lot with hardware and software; in an interview, quote your own measurements from weeks 1 to 3 instead and name the machine.

| Stage | Order of magnitude | How you measure it |
|---|---|---|
| Wire to user space, kernel UDP | microseconds | NIC hardware timestamp vs. `recv` return (day 18 used `SO_TIMESTAMPNS`) |
| Wire to user space, kernel bypass | around a microsecond or less | same, with the bypass stack's timestamps |
| Decode + book update | tens to hundreds of ns | TSC around the call, histogram (day 12) |
| SPSC hop between cores | tens of ns to a few hundred | day 7 ping-pong |
| Strategy + risk | tens to hundreds of ns | TSC, histogram |
| Encode + send to wire | about a microsecond (kernel) | TSC before encode vs. TX hardware timestamp |
| **Tick to trade** | the sum, measured on the wire | timestamps from a switch tap on the incoming packet and the outgoing order |

The interview point is the method: timestamp at every boundary with the TSC, carry the timestamps in the event, histogram per stage on a housekeeping thread, and validate end to end on the wire with a tap, because only the wire is the truth.

### Failure modes (they will ask "what happens if...")
| Event | Response |
|---|---|
| Gap on both feed lines | Mark the instrument stale, stop quoting it, cancel its orders, recover from snapshot or retransmit, then resume. |
| Feed silent (no heartbeat) | Same as a gap after a timeout. Silence is not "nothing happened". |
| Order gateway disconnect | Stop new orders, reconnect, replay or gap fill sequence numbers, reconcile every open order (exec reports, order status, drop copy), resume. Assume the worst-case position until reconciled. |
| Exec report for an unknown order | Halt that strategy, raise an alert: your state and the venue's disagree. |
| Strategy sends too many orders | Rate limit in the risk gate and in the gateway (venue throttles too), and an automatic kill switch on a breach. |
| Fat finger (wrong price or size) | Price collar against a reference, max order size, max notional. |
| Position or loss limit hit | Kill switch: cancel all, block new orders, require a human to re-enable. |
| A core thread stalls (bug, page fault, SMI) | A watchdog on a housekeeping core sees the heartbeat counter stop and cancels everything through a separate session or the venue's mass cancel. |
| Bad deploy | Replay yesterday's recorded inputs through the new build and diff the outputs before it goes live. |

### Variants you should be able to design
- **Market maker on one venue:** quote both sides, inventory skew, cancel fast on adverse moves (day 27).
- **Cross-venue arbitrage:** two feed handlers, one core sees both books, the risk gate checks the combined position, two gateways.
- **Many instruments (thousands):** shard instruments across core threads by symbol; each core owns its books; one gateway per venue session; risk limits per shard plus a global limit with a slower aggregated check.
- **Multicast to many internal consumers:** the feed handler publishes normalised events into shared memory (one writer, many readers with seqlocks or a broadcast ring).

## 2. Exercise: the risk gate (90 min)

Implement `RiskGate` in `solution.hpp`. It sits on the core thread, so the rules are the hot-path rules: no allocation after construction, no clock reads (use the order's event time), no exceptions, and integer arithmetic that cannot overflow. The tests check every verdict, the order of the checks, the sliding rate window, worst-case position with open orders, overflow, and count heap allocations.

Hints:
- Worst-case position means every open order on that side fills. Buys and sells are checked separately: a resting sell does not make room for a buy.
- `price * qty > max_notional` overflows for large inputs. Rearrange it so it cannot (both are positive).
- The rate window only needs the timestamps of the last `max_orders_per_window` accepts: a fixed ring allocated in the constructor.

```bash
cmake --build build --target day22_test && ./build/day22_test
```

**Done when:** tests pass under ASan+UBSan, and you can draw the diagram above, give a latency budget with how you would measure it, and answer every failure mode, in 20 minutes without notes.

## 3. Say it out loud

<details><summary>1. Why is the risk check inside the core thread and not a separate service?</summary>
Because it must run on every order before it leaves, and a hop to another thread or process adds latency and a failure point. It is a few integer compares on state the core already owns. Slower checks (aggregate firm-wide exposure) run elsewhere and push limits into the gate.
</details>

<details><summary>2. How do you replay a trading day?</summary>
Record every input to the core (decoded events with their sequence numbers and timestamps, exec reports, config changes) on the way in. The core uses only event time and seeded randomness, so feeding the recording back produces the same outputs bit for bit. Diff outputs to test a change or explain an incident.
</details>

<details><summary>3. Where do you log, and how, without hurting latency?</summary>
The hot thread writes a small binary record (format id plus arguments) into its own SPSC ring and moves on. A housekeeping thread drains the rings, formats text, writes files. If the ring is full, drop and count drops rather than block.
</details>

<details><summary>4. How would you scale to 5,000 instruments?</summary>
Shard by instrument across several core threads, each owning its books and strategy state, each with its own rings from the feed handlers. Partition feeds so a feed handler only forwards the symbols a shard needs. Per-shard risk limits, with a global limit enforced by a slower aggregator that can lower shard limits or hit the kill switch.
</details>

<details><summary>5. Your tick-to-trade got worse by 2 µs after a release. How do you find it?</summary>
Compare the per-stage histograms before and after to see which boundary moved. If none moved, look outside the process: host tuning (cmdline, IRQ affinity, governor), NIC firmware, switch. Then profile the stage that moved, and replay the same recorded input through both builds to compare on equal terms.
</details>

<details><summary>6. What does the kill switch do exactly?</summary>
Blocks all new orders in the risk gate, cancels every open order (per order, plus the venue's mass cancel where it exists), and stays on until a human re-enables it. It is triggered by an operator, by limits (position, loss, order rate, reject rate), or by a watchdog when a thread stops making progress.
</details>
