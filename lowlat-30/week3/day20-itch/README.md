# Day 20 · Market microstructure and a Nasdaq ITCH 5.0 feed handler

**Why trading firms care:** low-latency engineers build the systems traders and researchers use, and interviews check that you understand what the bytes mean: who trades with whom, why queue position matters, what a market maker earns and loses. Then the code: turn a real ITCH stream into a correct book.

## 1. Concepts (45 min)

### Who is in the market
- **Market makers** post bids and offers on both sides and earn the **spread** (plus exchange **rebates** for providing liquidity). They lose to **adverse selection**: they get filled exactly when someone better informed knows the price is about to move. And they carry **inventory risk**.
- **Takers** cross the spread for immediacy and pay the spread plus a taker fee.
- **Arbitrageurs** trade the same or related instruments across venues or products (ETF vs basket, future vs cash, the same stock on two exchanges) when prices get out of line. This is where pure speed matters most.
- **Statistical / directional traders** predict short-term moves from order flow and other signals.

### Order types and terms
| Term | Meaning |
|---|---|
| Limit order | Buy at or below / sell at or above a price; rests if it does not fill |
| Market order | Fill now at any price (walks the book) |
| IOC / FAK | Immediate-or-cancel: fill what you can now, cancel the rest |
| FOK | Fill-or-kill: all now or nothing |
| Post-only | Rest only; reject (or reprice) if it would take liquidity |
| Iceberg | Shows only part of its size; refills from hidden quantity (loses priority on each refill) |
| Spread | best ask − best bid |
| Tick size | Minimum price increment |
| Top of book / BBO | Best bid and offer; NBBO across all US exchanges |
| Queue position | How much size is ahead of you at your price |
| Auctions | Opening/closing crosses where orders match at a single price |

### Why queue position is worth money
At a given price level, the order at the **front** gets filled by small random flow and can cancel if the market turns. The order at the **back** only gets filled when a large aggressive order sweeps the whole level, which is often right before the price moves through it. So the same price is worth more at the front: being fast to join a new level is valuable, and so is being fast to cancel.

### ITCH 5.0 in one page
Nasdaq TotalView-ITCH is a **market-by-order** (L3) feed: every add, execution, cancel and replace of every displayed order. Big-endian binary. Every message has `type (1) | stock locate (2) | tracking (2) | timestamp ns since midnight (6)` then type-specific fields. Delivered over MoldUDP64 (multicast, with the sequence numbers you handled yesterday). Nasdaq's sample files use a simple framing: each message preceded by a **2-byte big-endian length**.

Messages you implement (offsets from the start of the message):
| Type | Name | Len | Fields after the 11-byte header |
|---|---|---|---|
| `A` | Add Order | 36 | ref u64 @11, side char @19, shares u32 @20, stock 8 @24, price u32 @32 |
| `F` | Add Order with MPID | 40 | as `A`, plus attribution 4 chars @36 |
| `E` | Order Executed | 31 | ref u64 @11, executed shares u32 @19, match number u64 @23 |
| `C` | Executed with Price | 36 | ref @11, shares @19, match @23, printable char @31, price u32 @32 |
| `X` | Order Cancel | 23 | ref @11, cancelled shares u32 @19 (partial cancel) |
| `D` | Order Delete | 19 | ref @11 |
| `U` | Order Replace | 35 | original ref @11, new ref @19, shares u32 @27, price u32 @31 |

Everything else (system events, stock directory, trades of non-displayed orders, NOII...) is skipped using the length prefix. Rules: an execution or cancel reduces the order's shares (delete when it reaches 0); delete removes it; **replace** = delete the original and add a new order with the new ref, same side, new price and shares, at the **back** of the queue.

Unlike your day 11 book, an ITCH book never matches: the exchange already did, and tells you the result.

## 2. Exercise (2 h)

In `solution.hpp`:
1. `parse_itch(stream, handler)`: walk length-prefixed messages, decode the seven types above and call the handler; skip other types; stop at a partial message at the end and return the number of bytes consumed (so the caller can keep the remainder, like day 15). A known type with the wrong length counts as an error and is skipped. You can reuse your day 2 `load_be` functions.
2. `ItchBook`: a handler that keeps the book for **one** stock locate (ignore other locates): `best_bid`, `best_ask`, `volume_at`, `order_count`. Reuse your day 11 data structures (levels + intrusive lists + id map), without the matching.

The tests build ITCH streams byte by byte, including split buffers and unknown message types, and check every event and the resulting book.

Stretch: download a Nasdaq sample file (`emi.nasdaq.com/ITCH/` has `*.NASDAQ_ITCH50.gz` files of several GB), stream it through your parser for one liquid stock, and measure messages per second.

**Done when:** tests pass under ASan+UBSan, and you can explain market making, adverse selection and queue position without notes.

## 3. Say it out loud

<details><summary>1. How does a market maker make and lose money?</summary>
Earns the bid-ask spread and liquidity rebates by trading with impatient takers. Loses to adverse selection (being filled by informed traders just before the price moves) and to inventory risk. Speed reduces adverse selection: cancel stale quotes before they are hit.
</details>

<details><summary>2. Why is queue position valuable?</summary>
Front-of-queue orders fill on small, mostly uninformed flow and can cancel in time; back-of-queue orders fill mostly when a large order sweeps the level, which often precedes an adverse move. Same price, very different expected value.
</details>

<details><summary>3. What is market-by-order vs market-by-price?</summary>
Market-by-order (L3, e.g. ITCH) sends every individual order event with its id; you rebuild the queue at each level. Market-by-price (L2, e.g. many futures feeds) sends aggregated quantity per level, often only the top N levels.
</details>

<details><summary>4. What happens on an ITCH Order Replace?</summary>
The original order is removed and a new one with a new reference number is added on the same side at the new price and size. It goes to the back of the queue at its level.
</details>

<details><summary>5. What is adverse selection, with an example?</summary>
Your resting bid gets filled because someone knows the price is dropping (they just saw a big sell on another venue). You bought right before it got cheaper. The fills you get are biased toward the bad ones.
</details>

<details><summary>6. What is latency arbitrage?</summary>
Trading on a price change seen on one venue before other participants (or your stale quotes) on another venue have reacted. It is why market makers race to cancel and why firms pay for microwave links and co-location.
</details>
