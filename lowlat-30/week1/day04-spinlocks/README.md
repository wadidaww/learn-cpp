# Day 4 · `std::atomic`, memory orders, spinlocks

**Why trading firms care:** threads on the hot path never block in the kernel. They hand data over with atomics. "Explain acquire/release" and "write a spinlock" are standard questions, and the follow-up is always "why is your spinlock slow under contention?"

## 1. Concepts (45 min)

### Two problems atomics solve
1. **Atomicity:** a read-modify-write (`fetch_add`, `exchange`, `compare_exchange`) happens as one indivisible step.
2. **Ordering / visibility:** without synchronisation, the compiler *and* the CPU may reorder memory operations, and another core may see your writes in a different order. Memory orders say which reorderings are forbidden.

A **data race** (two threads touch the same non-atomic location, at least one writes, no happens-before between them) is UB. `volatile` does not fix it.

### The memory orders
| Order | Guarantees | Typical use |
|---|---|---|
| `relaxed` | Atomic, no ordering with other locations | Counters, statistics |
| `release` (stores) | All my earlier reads/writes stay before this store | Publishing data: "data ready" flag |
| `acquire` (loads) | All my later reads/writes stay after this load | Consuming the flag, then reading the data |
| `acq_rel` (RMW) | Both | Lock-free RMW in the middle of a chain |
| `seq_cst` (default) | acquire/release + one global order of all seq_cst ops | When you need all threads to agree on an order (e.g. Dekker/Peterson) |

**Happens-before:** if thread A does a release store to `x` and thread B does an acquire load of `x` that reads that value, then everything A did before the store happens-before everything B does after the load.

```cpp
int data;                         // plain
std::atomic<bool> ready{false};

// producer                            // consumer
data = 42;                             while (!ready.load(std::memory_order_acquire)) {}
ready.store(true, std::memory_order_release);   assert(data == 42);  // guaranteed
```
With `relaxed` on either side, the assert can fail (on ARM in practice; on x86 the compiler can still reorder).

### What x86 actually emits (TSO model)
- Loads are not reordered with loads, stores not with stores, and loads not with older stores **except** a store followed by a load to a different address (store buffer).
- So on x86: acquire load = plain `mov`; release store = plain `mov`; seq_cst store = `xchg` (or `mov`+`mfence`); RMWs = `lock`-prefixed instructions (`lock xadd`, `lock cmpxchg`), always a full barrier.
- On ARM, acquire/release become `ldar`/`stlr`. Code that "works on x86" with relaxed can break on ARM.

### compare_exchange
```cpp
T expected = cur;
while (!a.compare_exchange_weak(expected, f(expected), std::memory_order_acq_rel, std::memory_order_relaxed)) {
    // expected now holds the current value; retry
}
```
`weak` may fail spuriously (fine in a loop, cheaper on LL/SC machines). `strong` does not fail spuriously.

### Spinlocks and cache coherence (MESI)
A cache line is Modified, Exclusive, Shared or Invalid in each core. Writing needs exclusive ownership, so every write invalidates the line in all other cores.

- **TAS lock:** `while (flag.exchange(true, acquire)) {}`. Every spin iteration is a write (RMW), so waiting cores fight over the line: the line ping-pongs and even the owner's unlock is delayed.
- **TTAS lock:** spin on a plain `load(relaxed)` until it looks free, *then* try `exchange`. Waiters share the line read-only and generate no traffic until the unlock.
- Add `_mm_pause()` (x86 `pause`) inside the spin: it saves power, frees resources for the sibling hyperthread, and avoids a memory-order pipeline flush when the loop exits.
- Spinlocks are right only when the critical section is tiny and threads are pinned to their own cores. If the owner is descheduled, waiters burn a whole time slice. That is why general code uses `std::mutex` (spins briefly, then sleeps in the kernel via futex).

## 2. Exercise (2 h)

In `solution.hpp`:
1. `TasSpinlock` with `lock`, `try_lock`, `unlock` using `std::atomic<bool>::exchange`.
2. `TtasSpinlock`: test-and-test-and-set plus `_mm_pause()` (use `__builtin_ia32_pause()` or include `<immintrin.h>`).
3. `Mailbox`: a single-slot handoff of one `int64_t` from one producer to one consumer, using a plain `int64_t` payload and an `std::atomic<bool>` flag with the **weakest correct** orders. `publish` must not overwrite an unconsumed value (return false instead).

Both locks must satisfy *BasicLockable* so `std::lock_guard` works.

Run the tests normally **and under ThreadSanitizer**:
```bash
cmake -S lowlat-30 -B build-tsan -DLL_SANITIZE=thread && cmake --build build-tsan --target day04_test && setarch -R ./build-tsan/day04_test
```
(`setarch -R` disables address randomisation, which some kernels need for TSan.)

Then try this on purpose: change the Mailbox orders to `relaxed` and rerun under TSan. It reports a data race. Explain why.

**Done when:** tests pass under TSan, the bench runs, and you can explain why TTAS beats TAS as threads increase.

## 3. Say it out loud

<details><summary>1. Explain acquire/release in one breath.</summary>
A release store makes all the thread's earlier writes visible to any thread whose acquire load reads that stored value. That pair creates happens-before, so the consumer sees the producer's data.
</details>

<details><summary>2. What does seq_cst add?</summary>
A single total order over all seq_cst operations that every thread agrees on. Needed for patterns like Dekker's (each thread writes its flag then reads the other's), where acquire/release allows both threads to read the old values.
</details>

<details><summary>3. Why is seq_cst store slower on x86?</summary>
x86 can reorder a store with a later load (store buffer). To forbid that, a seq_cst store must drain the store buffer: `xchg` or `mov`+`mfence`, costing tens of cycles. A release store is a plain mov.
</details>

<details><summary>4. Why is TTAS faster than TAS?</summary>
TAS writes on every spin, so the cache line bounces between waiting cores in Modified state. TTAS spins on reads; the line stays Shared in all waiters' caches with no traffic until the owner writes on unlock.
</details>

<details><summary>5. When is a spinlock a bad idea?</summary>
When critical sections are long, when there are more threads than cores, or when threads are not pinned: a preempted owner makes everyone spin for a full time slice. Use a mutex (futex) there.
</details>

<details><summary>6. Does volatile make code thread-safe?</summary>
No. It stops the compiler from removing or merging accesses to that variable but gives no atomicity and no ordering with other memory, and a race on it is still UB. It is for memory-mapped I/O.
</details>

<details><summary>7. compare_exchange_weak vs strong?</summary>
Weak may fail spuriously even if the value matched (LL/SC architectures), so use it in retry loops. Strong only fails if the value differed; use it for one-shot attempts.
</details>

<details><summary>8. What does `lock` prefix do on x86?</summary>
Makes the read-modify-write atomic by holding the cache line exclusively for the duration, and acts as a full memory barrier.
</details>
