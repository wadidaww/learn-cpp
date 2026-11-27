# Day 11 · The limit order book, from scratch

**Why trading firms care:** this is the one. Almost every HFT loop includes "implement an order book" (live coding, 45–60 minutes) or "design a fast one" (whiteboard). Your resume claims one, so you must be able to build it, explain every choice, and measure it. Today you write it. Days 12–14 measure, profile and defend it.

## 1. Concepts (45 min)

### What the book does
Two sides of resting limit orders. **Bids** (buy) sorted best = highest price first; **asks** (sell) best = lowest price first. Within a price level, **price-time priority**: earlier orders fill first (FIFO).

Operations and the complexity you should aim for:
| Operation | Meaning | Target |
|---|---|---|
| add (passive) | rests at its price | O(1) |
| add (aggressive) | crosses the spread: match against the other side, rest any remainder | O(fills) |
| cancel | remove by order id | O(1) |
| reduce / modify qty down | keeps time priority | O(1) |
| modify price or qty up | loses priority: cancel + add | O(1) |
| best bid / ask | top of book | O(1) |

Real exchange rule to mention: an order that increases quantity or changes price goes to the **back** of the queue. A quantity decrease keeps its place.

### Data structure choices (the interview discussion)
**1. `std::map<Price, Level>` per side**
- Easy, sorted, O(log L) per new level.
- Each level is a separate heap node: pointer chasing, cache misses, allocation. Fine for a first version, slow for HFT.

**2. Array indexed by price (dense ladder)**
- Prices are integer ticks inside a known band (`min_price..max_price`, e.g. today's price limits). `levels[px - min_price]` is O(1) with no hashing.
- Track `best_bid` / `best_ask` indices. On a level becoming empty, scan to the next non-empty level. Scans are short in practice because liquidity clusters near the top; a bitmap of non-empty levels (find next set bit with `std::countr_zero`) makes it fast in the worst case.
- Cost: memory for every tick in the band, and a re-centre step if price leaves the band.
- This is what most fast books do.

**3. Sorted `std::vector` of levels**, best price at the **end** so insertions/removals near the top are cheap (few elements move). Good when the number of levels is small; very cache friendly.

**Per level: FIFO of orders.** Use an **intrusive doubly linked list** threaded through the order objects (each order stores `prev`/`next` indices) so cancel removes in O(1) without searching. Keep `total_qty` per level so "volume at price" is O(1).

**Order id → order:** a hash map. `std::unordered_map` (node-based: allocation, pointer chase) is OK for v1 with `reserve`; the fast version is open addressing with linear probing over a power-of-two table, or, if ids are dense (many venues assign sequential ids), a direct array.

**Order storage:** your day 10 `ObjectPool`, with 32-bit indices instead of pointers.

### The matching loop
```
add(id, side=Buy, px, qty):
    while qty > 0 and asks not empty and best_ask <= px:
        level = asks[best_ask]
        maker = level.front
        traded = min(qty, maker.qty)
        emit Fill{maker.id, id, best_ask, traded}     // trade at the MAKER's price
        qty -= traded; maker.qty -= traded; level.total -= traded
        if maker.qty == 0: unlink + free maker
        if level empty: advance best_ask to next non-empty level (or none)
    if qty > 0: rest at px on the bid side (append to that level's tail)
```
Sell is the mirror image. Two points interviewers check: **the trade price is the resting order's price**, and **the taker may sweep several levels**.

### Edge cases to handle
Duplicate order id, price outside the band, qty ≤ 0, cancel of an unknown id, reduce to zero (= cancel), reduce to more than current (reject), self-crossing on the same side never happens, an order that fully fills on arrival never rests, reuse of an id after the order is gone.

## 2. Exercise (2 h, it will take longer, finish it tomorrow morning if needed)

Implement `ll::OrderBook<Listener>` in `solution.hpp` with the given interface. Constraints:
- Prices are integer ticks in `[min_price, max_price]` set at construction; `max_orders` is the most live orders at once.
- All memory is allocated in the constructor. **No allocation in add/cancel/reduce** (a `std::unordered_map` that you `reserve` is acceptable for v1; replace it for the stretch goal).
- Fills are reported by calling `listener.on_fill(const Fill&)` in the order they happen.

The tests include a **differential test**: 200,000 random operations run on your book and on a slow but obviously correct model (`std::map` + `std::deque`), and every fill, best price and level volume must match. If it fails, the test prints the first step that differs.

**Done when:** all tests pass under ASan+UBSan, and you can draw your data structure from memory and explain why you chose it over the other two.

Stretch:
1. Replace the hash map with your own open-addressing table (power-of-two capacity, linear probing, tombstones or backward-shift delete).
2. Add a bitmap of non-empty levels and find the next best level with `std::countr_zero`.

## 3. Say it out loud

<details><summary>1. Which data structure would you use for the price levels and why?</summary>
A dense array indexed by tick within the day's price band, with cached best bid/ask indices and a bitmap to find the next non-empty level. O(1) lookups, no allocation, cache-friendly, and activity clusters near the top so scans are short. std::map is easier but every level is a heap node.
</details>

<details><summary>2. How is cancel O(1)?</summary>
id → order via a hash map, then unlink the order from its level's intrusive doubly linked list using its stored prev/next, and subtract its qty from the level total.
</details>

<details><summary>3. At what price does a trade execute?</summary>
At the resting (maker) order's price. A buy at 105 that hits an ask at 103 trades at 103: price improvement for the taker.
</details>

<details><summary>4. Which modifications lose time priority?</summary>
Any price change or quantity increase: it becomes a cancel + new order at the back of the queue. A quantity decrease keeps its position.
</details>

<details><summary>5. What happens when the price leaves your array band?</summary>
Either reject (exchanges have price bands and limit up/down anyway) or re-centre: allocate a new window at startup sized generously, or keep a sparse overflow map for far-away levels, since they are rarely touched.
</details>

<details><summary>6. How do you test an order book?</summary>
Unit tests for each rule, then a differential test against a simple reference model on millions of random operations, then replay of real exchange data (ITCH) checking the book against the exchange's own top-of-book, then fuzzing.
</details>

<details><summary>7. L1 vs L2 vs L3 book?</summary>
L1: best bid and ask only. L2: aggregated quantity per price level. L3 (market by order): every individual order with its id, which is what ITCH gives you and what you build here.
</details>
