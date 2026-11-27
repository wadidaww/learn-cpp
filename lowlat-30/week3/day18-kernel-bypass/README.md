# Day 18 · The packet's path, kernel bypass, timestamps, tick-to-trade

**Why trading firms care:** "what happens between the wire and your `recv()`?" and "how do firms make that faster?" are standard senior questions. You do not need to have used DPDK or Onload, but you must explain what they remove and why. Today is mostly reading and one write-up, plus a bench that measures part of the kernel path on your machine.

## 1. Concepts (90 min today, there is less to build)

### The kernel receive path, step by step
1. Packet arrives at the NIC. The NIC **DMAs** it into a receive ring buffer in host memory (descriptors point to pre-allocated buffers).
2. The NIC raises an **interrupt** (moderated/coalesced: it may wait to batch several packets, adding latency).
3. The kernel's interrupt handler schedules **NAPI** polling in a softirq. The driver builds an `sk_buff` per packet.
4. The packet climbs the stack: IP (routing, checksum), UDP (lookup of the socket by port), netfilter hooks.
5. It is queued on the **socket receive buffer**. If a thread is blocked in `recv`/`epoll_wait`, it is woken (scheduler, possibly a different core: IPI, cold cache).
6. Your `recv()` syscall **copies** the data from kernel memory into your buffer and returns.

Every step costs something: interrupt, softirq, allocation of the skb, protocol processing, wake-up, syscall, copy. A rough order of magnitude for a kernel UDP receive is a few microseconds end to end on a tuned box, and much worse on the tail.

### What kernel bypass removes
The NIC's rings are mapped **directly into user space**, and your thread **polls** them. No interrupt, no skb, no syscall, no copy (zero-copy: you parse in the DMA buffer).
| Technology | What it is |
|---|---|
| **Solarflare/AMD OpenOnload** | A user-space TCP/UDP stack loaded with `LD_PRELOAD`: your normal socket code runs unchanged but bypasses the kernel. Very common in trading. |
| **ef_vi** | Solarflare's raw layer-2 API under Onload: you get frames from the ring and parse Ethernet/IP/UDP yourself. Lowest software latency. |
| **DPDK** | Intel-originated framework: poll-mode drivers take the NIC away from the kernel (VFIO), huge pages, batch processing. More common in telecom and in-house stacks. |
| **NVIDIA/Mellanox VMA, RDMA (ibverbs)** | Similar ideas for ConnectX NICs; RDMA offloads transport to the NIC. |
| **AF_XDP / XDP** | The Linux kernel's own fast path: an eBPF program steers packets to a user-space ring before the stack. |
| **Exablaze / Cisco ExaNIC, FPGA NICs** | Very low-latency NICs; FPGAs can parse market data and even send orders in hardware (sub-microsecond tick-to-trade). |

Trade-offs to mention: a dedicated, spinning core per ring; you own reliability (TCP in user space is hard); special hardware and drivers; debugging is harder (no tcpdump on a bypassed interface without extra tools).

### Timestamps
- **Software timestamps** (`SO_TIMESTAMPNS`): the kernel records when the packet entered the stack. The difference between that and when `recv` returns is the kernel→user part of your latency (today's bench).
- **Hardware timestamps** (`SO_TIMESTAMPING` with `SOF_TIMESTAMPING_RX_HARDWARE`): the NIC stamps the packet on arrival, with clocks disciplined by **PTP** (IEEE 1588). Firms measure "wire-to-wire" latency with hardware timestamps or with a network tap and a capture appliance, never with `rdtsc` alone.
- Regulation (MiFID II in Europe) requires clocks synchronised to UTC within 100 µs for HFT, with timestamps of 1 µs granularity or better.

### Tick-to-trade
Time from the market data packet arriving at your NIC to your order packet leaving it. Components: NIC + PCIe in (~hundreds of ns), feed decode, book update, strategy decision, risk check, order encode, NIC out. Public talks put top software systems at single-digit microseconds and FPGA systems well under one microsecond. Say "order of magnitude, from public talks" when you quote these: the real numbers are firm secrets.

Beyond the server: **co-location** in the exchange's data centre, cross-connect cable length (light in fibre ≈ 5 ns per metre), switch latency (cut-through switches ~ hundreds of ns), and microwave/millimetre-wave links between data centres (faster than fibre because light travels ~50% faster in air than in glass).

## 2. Exercise

1. **Bench (30 min).** `bench.cpp` sends UDP packets over loopback and uses `SO_TIMESTAMPNS` to split each packet's journey into "send → kernel receive timestamp" and "kernel receive → `recv()` returned", once with a blocking receiver and once with a busy-polling one (`MSG_DONTWAIT` spin). Run it, and explain why the second part shrinks with busy polling.
2. **Write-up (60 min).** In your own repo, write one page: "Tick-to-trade: where each microsecond goes". Draw the path from the exchange's matching engine to your order arriving back at the exchange. For each box give: what happens, an order-of-magnitude cost, and how firms reduce it. Include: wire/co-lo, switch, NIC, kernel or bypass, decode, book, strategy, risk, encode, send, and the return path.
3. **Read** the Onload or ef_vi user guide overview, or the DPDK "Getting Started" overview, enough to explain one of them in 60 seconds.

**Done when:** you can talk through your page in 3 minutes without notes.

## 3. Say it out loud

<details><summary>1. What happens between the wire and recv()?</summary>
NIC DMA into a ring, interrupt (coalesced), NAPI softirq builds an skb, IP/UDP processing and socket lookup, queue on the socket buffer, wake the blocked thread, then recv copies the data to user space. Each step adds latency and jitter.
</details>

<details><summary>2. What does kernel bypass remove?</summary>
Interrupts, skb allocation, the kernel protocol stack, syscalls, the wake-up, and the copy. The application polls the NIC ring mapped into its own memory and parses packets in place.
</details>

<details><summary>3. Onload vs ef_vi vs DPDK?</summary>
Onload: a user-space socket stack via LD_PRELOAD, no code changes. ef_vi: Solarflare's raw frame API, lowest latency, you parse headers yourself. DPDK: a general framework with poll-mode drivers and huge pages, owning the NIC completely.
</details>

<details><summary>4. How do firms measure latency end to end?</summary>
Hardware timestamps on NICs or switches, PTP-synchronised clocks, or passive network taps feeding a capture appliance that timestamps both the market data in and the order out. Software rdtsc timing is for code paths, not the wire.
</details>

<details><summary>5. Why do HFT firms use microwave links?</summary>
Light travels about 50% faster in air than in optical fibre, and microwave paths are straighter than fibre routes. Between, say, Chicago and New Jersey that saves milliseconds, at the cost of bandwidth and weather sensitivity.
</details>

<details><summary>6. What is an FPGA used for in trading?</summary>
Parsing market data, maintaining a book for a few instruments, and triggering pre-computed orders in hardware, reaching tick-to-trade well under a microsecond. Software sets the parameters; the FPGA does the fast reaction.
</details>
