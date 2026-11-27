# Day 8 · The memory hierarchy: caches, TLB, prefetching, huge pages

**Why trading firms care:** at low latency, most of your time goes to waiting for memory, not computing. "How long is an L1 hit vs a DRAM access?" is asked in almost every low-latency interview, and the follow-up is "how would you measure it?". Today you measure it yourself.

## 1. Concepts (45 min)

### Numbers to know (typical modern x86 server, order of magnitude)
| Level | Size per core | Latency | |
|---|---|---|---|
| Register | | 0 cycles | |
| L1d | 32–48 KB | ~4–5 cycles (~1 ns) | private |
| L2 | 1–2 MB | ~12–15 cycles (~4 ns) | private |
| L3 (LLC) | 1.5–4 MB per core, shared | ~40–70 cycles (~10–20 ns) | shared by the socket |
| DRAM | GBs | ~200–350 cycles (~80–100 ns) | |
| Other socket's DRAM (NUMA) | | +50–100% | |
| Cross-core cache-line transfer | | ~40–100 ns | coherence |

Say "about" and give the order of magnitude. Then say you measure it on the target machine.

### Why the hierarchy matters
- Data moves in **64-byte lines**. Touching one byte loads 64.
- **Spatial locality:** use the rest of the line (arrays of structs you iterate, packed hot fields).
- **Temporal locality:** reuse data while it is still cached (small working set).
- **Hardware prefetchers** detect sequential and strided access and fetch ahead. Linear scans of a big array can run near memory bandwidth; random pointer chasing cannot, because each address depends on the previous load (no memory-level parallelism).
- That is why `std::map` (a red-black tree, one node per allocation, random addresses) is slow for an order book and `std::vector` / flat arrays are fast.

### TLB and huge pages
- Virtual→physical translation is cached in the **TLB** (~64 L1 entries, ~1.5–2K L2 entries). With 4 KB pages, 1,536 entries cover only 6 MB.
- A TLB miss triggers a page walk (up to 4 dependent memory reads, often cached, ~20–30+ cycles).
- **Huge pages** (2 MB or 1 GB) cover far more memory per entry. HFT systems put books, pools and rings in huge pages (`mmap` with `MAP_HUGETLB`, or transparent huge pages via `madvise(MADV_HUGEPAGE)`).
- Also **pre-fault** memory at startup (touch every page, or `MAP_POPULATE`, plus `mlockall`) so the hot path never takes a page fault (a minor fault is ~1 µs, a major one is much worse).

### Write behaviour
- Writes go to the store buffer, then L1. Writing to a line you do not own needs a **read-for-ownership** (RFO): the line is fetched first.
- Non-temporal stores (`_mm_stream_si128`) bypass caches for large buffers you will not read again (logging, recording).

### How to measure latency of a level: pointer chasing
Make an array of N slots where each slot holds the index of the next slot, in a **single random cycle**. Then `i = next[i]` repeated: each load depends on the previous one, prefetchers cannot guess, and time per step equals load latency for a working set of N × 8 bytes. Plot ns/step against working set size: you see steps at the L1, L2 and L3 sizes.

To make one random cycle use **Sattolo's algorithm** (a Fisher–Yates variant where `j` is drawn from `[0, i)` instead of `[0, i]`), which always produces one cycle through all elements.

## 2. Exercise (2 h)

In `solution.hpp`:
1. `make_cycle(n, seed)` returns a `std::vector<std::uint32_t>` where following `next[i]` from 0 visits every index exactly once before returning to 0. Use Sattolo's algorithm with the provided `SplitMix64` (do not use `std::rand` or `<random>` distributions).
2. `chase(next, steps)` follows the chain `steps` times from 0 and returns the final index.
3. `sum_sequential(data)` and `sum_strided(data, stride)` (stride in elements, visiting every element exactly once).

Then run `bench` and plot (spreadsheet or Python) **ns per load vs working set**. Mark where L1, L2 and L3 end on your machine (`lscpu` or `getconf -a | grep CACHE`) and check that the steps line up.

**Done when:** tests pass, you have the plot, and you can explain the sequential-vs-chase gap with the word "prefetcher".

Stretch: rerun the chase with the array in a 2 MB huge page (`mmap(... MAP_HUGETLB)` after `echo 64 | sudo tee /proc/sys/vm/nr_hugepages`) and see the DRAM-sized points improve.

## 3. Say it out loud

<details><summary>1. Rough latencies of L1, L2, L3, DRAM?</summary>
About 1 ns, 4 ns, 10–20 ns, 80–100 ns. In cycles at ~3 GHz: 4, 12–15, 40–70, 250–350. Always followed by "I'd measure it on the target box".
</details>

<details><summary>2. Why is a linked list slow even if every node is in L3?</summary>
Each next pointer is only known after the previous load completes, so loads cannot overlap (no memory-level parallelism) and prefetchers cannot predict the addresses. Each hop pays full latency.
</details>

<details><summary>3. Why would you use huge pages?</summary>
Fewer TLB misses: one 2 MB entry covers what 512 4 KB entries would. Also fewer page faults at warm-up. Used for large books, pools and rings.
</details>

<details><summary>4. How would you measure cache latency?</summary>
Random pointer chase over a single-cycle permutation, sweeping the working set size, timing many dependent loads, pinned to one core. The time per load jumps at each cache size.
</details>

<details><summary>5. What is a read-for-ownership?</summary>
Before writing to a line it does not hold exclusively, a core must fetch it and invalidate other copies. So writes cost a read too, and writes to shared lines cause coherence traffic.
</details>

<details><summary>6. What is NUMA and why does it matter?</summary>
Each socket has its own memory controller. Accessing the other socket's memory is slower. Pin threads and allocate their memory on the same node (`numactl --cpunodebind --membind`), and keep the NIC on that node too.
</details>

<details><summary>7. Why pre-fault memory?</summary>
The first touch of a page triggers a page fault (kernel entry, zeroing a page): microseconds. Touch everything at startup and mlock it so the hot path never faults.
</details>
