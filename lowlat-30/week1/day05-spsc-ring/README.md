# Day 5 · False sharing and the SPSC ring buffer

**Why trading firms care:** the feed thread hands decoded updates to the strategy thread through a single-producer single-consumer ring. It is the most common lock-free structure in HFT and the most common whiteboard question. You must be able to write it from memory in 15 minutes and defend every memory order.

## 1. Concepts (45 min)

### Cache lines and false sharing
- Caches move data in **64-byte lines** (x86; Apple M-series uses 128). Coherence is tracked per line, not per variable.
- **False sharing:** thread A writes `x`, thread B writes `y`, and `x` and `y` share a line. Each write invalidates the other core's copy, so the line bounces (~40–100 ns per transfer across cores) even though the threads share no data.
- Fix: put each hot, independently written variable on its own line: `alignas(64)`. `std::hardware_destructive_interference_size` names this constant, but GCC warns it can change between compiler versions (ABI), so many codebases hard-code 64. Intel's adjacent-line prefetcher pulls pairs of lines, so some firms pad to 128.

### The SPSC ring
```
 capacity = 8 (power of two), mask = 7
 head_ (consumer owns)          tail_ (producer owns)
     v                               v
 [ . | A | B | C | D | . | . | . ]      size = tail - head
```
- **Producer** owns `tail_`; **consumer** owns `head_`. Each index is written by only one thread, so no RMW (no `lock` instructions) is needed. Only loads and stores.
- Use **monotonically increasing** unsigned indices and `index & mask` to find the slot. Then `tail - head` is the size even after wrap-around (unsigned arithmetic is modular), and you can use all `capacity` slots (no "waste one slot" trick needed).
- Capacity is a power of two so `& mask` replaces `%` (a division is 20–40 cycles).

**Push (producer):**
1. `t = tail_.load(relaxed)` (only I write it).
2. `h = head_.load(acquire)`; if `t - h == capacity`, full. *Acquire* so the consumer's read of that slot happened before I overwrite it.
3. Write `buf_[t & mask]`.
4. `tail_.store(t + 1, release)`. *Release* publishes the slot contents.

**Pop (consumer):** mirror image: `h = head_.load(relaxed)`, `t = tail_.load(acquire)`, empty if equal, read slot, `head_.store(h + 1, release)`.

### The optimisation interviewers look for: cached indices
Every push loads `head_`, which lives on a line the consumer keeps writing. That load is a cache miss whenever the consumer has moved. Fix: the producer keeps a **private copy** `head_cache_` and only reloads the real `head_` when the cached value says the ring is full. Same for the consumer with `tail_cache_`. In steady state each side then touches the other's line only once per lap. This is the difference between ~50 ns and ~5–10 ns per item across cores.

Layout (each group on its own line):
```
alignas(64) std::atomic<uint64_t> tail_;   uint64_t head_cache_;   // producer's line
alignas(64) std::atomic<uint64_t> head_;   uint64_t tail_cache_;   // consumer's line
alignas(64) T buf_[N];
```

### Other details worth naming
- Indices as `uint64_t` never wrap in practice (584 years at 1 G ops/s). With `uint32_t` they wrap, and the arithmetic still works because capacity divides 2³².
- Batching: pop many items, store `head_` once. Fewer cross-core writes.
- MPSC/MPMC need RMW (`fetch_add` or CAS) to claim slots, plus per-slot sequence numbers (Vyukov's bounded MPMC queue). Know that it exists and why it is slower.

## 2. Exercise (2 h): `ll::SpscRing<T, N>`

In `solution.hpp`:
- `try_push(const T&)`, `try_push(T&&)`, `try_pop(T&)`, `size_approx()`, `empty_approx()`, `capacity()`.
- `static_assert` that N is a power of two.
- Use all N slots.
- Producer and consumer indices on separate cache lines, with cached copies.
- No locks, no RMW instructions, weakest correct orders.

Run tests normally and under TSan (see day 4).

**Done when:** tests pass under TSan; on godbolt `try_push` contains no `lock` prefix and no `mfence`; you can write it on paper in 15 minutes (time yourself, this is Mock 1).

## 3. Say it out loud

<details><summary>1. Why does an SPSC ring need no compare-and-swap?</summary>
Each index has exactly one writer. The producer only stores tail and the consumer only stores head; each only reads the other's. Plain loads/stores with acquire/release are enough.
</details>

<details><summary>2. Walk through the memory orders in push.</summary>
Load own tail relaxed (only I write it). Load head acquire so the consumer's read of the slot I am about to reuse is complete. Write the slot. Store tail+1 with release so the slot write is visible before the consumer sees the new tail.
</details>

<details><summary>3. What is false sharing and how do you prove it is happening?</summary>
Independent variables written by different cores on the same cache line, causing coherence traffic. Prove it with `perf c2c` (shows contended lines and HITM events) or by padding and measuring the change.
</details>

<details><summary>4. Why power-of-two capacity?</summary>
`i & (N-1)` instead of `i % N`: one AND instead of a division, and the modular index arithmetic stays correct when the counters wrap.
</details>

<details><summary>5. Why cache the other side's index?</summary>
Reading the other thread's index pulls its cache line across cores. With a local cached copy you only re-read it when the cached value says full (or empty), so in steady state cross-core traffic drops to about once per lap.
</details>

<details><summary>6. Full vs empty with monotonically increasing indices?</summary>
Empty when tail == head. Full when tail - head == N. No need to waste a slot.
</details>

<details><summary>7. How do you make it multi-producer?</summary>
Producers claim slots with fetch_add on tail (or CAS), and each slot carries a sequence number that says whether it is free or filled for this lap (Vyukov MPMC). More contention, RMW on a shared line, so slower than SPSC. In HFT you usually prefer one SPSC per producer.
</details>
