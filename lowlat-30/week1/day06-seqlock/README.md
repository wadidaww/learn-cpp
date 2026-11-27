# Day 6 · Seqlocks, ABA, and the lock-free vocabulary

**Why trading firms care:** a seqlock lets one writer publish a small snapshot (top of book, a position, a config) that many readers read **without ever writing to shared memory**. Readers never slow the writer. It is in the Linux kernel (clock time) and in most HFT codebases. It is also a famous C++ memory-model trap, which makes it a great senior-level interview question.

## 1. Concepts (45 min)

### How a seqlock works
A sequence counter `seq` plus the data.
- **Writer:** `seq++` (now odd = "write in progress"), write data, `seq++` (even again).
- **Reader:** read `seq` (s0); if odd, retry. Copy data. Read `seq` again (s1). If `s0 != s1`, a write overlapped: discard and retry.

Readers only load, so the line holding `seq` stays Shared in every reader's cache: zero coherence traffic until the writer writes. Compare with a reader-writer lock, where every reader does an RMW on the lock word.

Trade-offs: one writer (or writers serialised by a separate lock); readers may retry forever under a constantly writing writer (livelock); data must be trivially copyable since a reader may copy a half-written object and throw it away.

### The C++ memory model trap
The textbook version reads the data with plain loads while the writer writes them. That is a **data race, so UB**, even though the reader discards the bad copy. The compiler may also move plain data reads past the second `seq` load. Correct C++ (Hans Boehm, "Can Seqlocks Get Along With Programming Language Memory Models?", 2012):

```cpp
// data stored as std::atomic<uint64_t> words, accessed with relaxed loads/stores
// writer (single thread)
auto s = seq.load(relaxed);
seq.store(s + 1, relaxed);
std::atomic_thread_fence(std::memory_order_release);   // odd seq visible before data writes
for (i...) words[i].store(src[i], relaxed);
seq.store(s + 2, release);                              // data visible before even seq

// reader
do {
    s0 = seq.load(acquire);                             // data reads stay after this
    if (s0 & 1) continue;
    for (i...) dst[i] = words[i].load(relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);// data reads stay before the next load
    s1 = seq.load(relaxed);
} while (s0 & 1 || s0 != s1);
```
On x86 every one of those relaxed atomics is a plain `mov` and the fences compile to nothing (they only stop compiler reordering), so this costs the same as the "racy" version while being correct. Say exactly that in the interview.

### Vocabulary you will be asked about
- **Lock-free:** some thread always makes progress, even if others are suspended. **Wait-free:** every thread finishes in a bounded number of its own steps. **Obstruction-free:** a thread running alone finishes.
- **ABA problem:** a CAS sees value A, another thread changes A→B→A, the CAS succeeds wrongly (classic in a lock-free stack where a node is freed and reused). Fixes: tagged pointers / version counters (double-width CAS), hazard pointers, epoch-based reclamation. Seqlocks and SPSC rings avoid ABA because indices only grow.
- **Memory reclamation** is the hard part of lock-free structures with pointers. HFT code avoids it with fixed pools and indices.
- **Livelock:** threads keep running but make no progress (seqlock reader under a busy writer).

## 2. Exercise (2 h): `ll::Seqlock<T>`

In `solution.hpp`, implement `store` (single writer), `load` (spins until a consistent copy) and `try_load` (one attempt) following the pattern above.
- `T` must be trivially copyable (constrain it).
- Store the data as `std::array<std::atomic<std::uint64_t>, words>` and move bytes with `std::memcpy` to/from a local `uint64_t` array.
- Readers must never write any shared memory.

Use it for a `TopOfBook { int64_t bid_px, bid_qty, ask_px, ask_qty; uint64_t seq_no; }`.

Note on TSan: GCC prints `'atomic_thread_fence' is not supported with '-fsanitize=thread'` (`-Wtsan`). TSan does not model standalone fences. Because every data access here is atomic, there is no data race for it to report anyway; the torn-read test is what checks your ordering. (Interview bonus: this is a known TSan limitation.)

**Done when:** tests pass under TSan (the multi-reader test checks for torn reads), and you can explain why the naive version is UB and why the correct version costs nothing extra on x86.

Stretch: benchmark reader throughput with 1, 2, 3 reader threads against a `std::shared_mutex` version.

## 3. Say it out loud

<details><summary>1. Why are seqlock readers fast?</summary>
They never write shared memory. The sequence counter's cache line stays Shared across all reader cores, so there is no coherence traffic until the writer updates it.
</details>

<details><summary>2. Why is the naive seqlock UB in C++?</summary>
Readers read non-atomic data concurrently with the writer's writes: a data race, which is undefined even if the result is discarded. The fix is relaxed atomic accesses for the data plus fences.
</details>

<details><summary>3. What do the fences cost on x86?</summary>
Nothing at runtime: acquire and release fences are compiler barriers only on x86 (TSO already gives those orderings). Relaxed atomic loads/stores of 8 bytes are plain movs.
</details>

<details><summary>4. Can a seqlock have two writers?</summary>
Not by itself: two writers could both make seq odd and interleave data. Serialise writers with a spinlock or a CAS on seq, or keep one writer by design.
</details>

<details><summary>5. What is the ABA problem?</summary>
A compare-and-swap succeeds because the value is the same as before, although it changed and changed back in between. Fix with a version tag alongside the pointer, or safe memory reclamation so a node is not reused while someone might still hold it.
</details>

<details><summary>6. Lock-free vs wait-free, with an example.</summary>
A CAS retry loop is lock-free: someone's CAS always succeeds, but one thread can starve. A fetch_add counter is wait-free on x86: every thread finishes in one instruction. The SPSC ring is wait-free for both sides.
</details>

<details><summary>7. When would you choose a seqlock over an SPSC ring?</summary>
When readers only need the latest state, not every update (top of book, positions, risk limits). A ring delivers every message in order; a seqlock overwrites, and readers skip intermediate versions.
</details>
