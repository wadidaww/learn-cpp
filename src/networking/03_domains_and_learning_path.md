# 03 — Domains, Architecture, and Practice Projects

Low-latency techniques show up in many domains.  The exact tradeoffs differ, but
the same questions repeat: where do packets wait, where do bytes copy, and where
do threads contend?

## Typical domains

- **Trading systems**: market-data feeds, order gateways, exchange connectivity,
  and risk checks where p99/p99.9 latency matters.
- **Game servers**: real-time state updates, interest management, hit detection,
  and jitter-sensitive UDP traffic.
- **Realtime telemetry**: high-rate metrics, vehicle data, robotics, industrial
  systems, and observability pipelines.
- **RPC systems**: service-to-service calls where tail latency affects user-facing
  request latency.
- **Exchange gateways**: protocol translation and validation between internal
  systems and external venues.

## C++ topics that matter

### Memory

Care about allocations in hot paths, object lifetime, cache locality, move
semantics, and when a custom allocator or pool is worth the complexity.

Avoid:

- Allocating one object per message.
- Copying buffers through many layers.
- Polymorphic designs in tight loops without a measured reason.

### Data structures

Prefer:

- Contiguous storage.
- Fixed-size buffers where possible.
- Bounded queues.
- Flat layouts with predictable memory access.

Be cautious with:

- Linked lists.
- Excessive `std::shared_ptr` ownership.
- `std::map` or tree-heavy hot paths.

### Error handling

Exceptions are often avoided inside hot networking loops because the fast path
should be branch-predictable and allocation-free.  Many systems use error codes,
`std::optional`, or expected-style returns at low levels, while still allowing
exceptions during setup or tests.

### Timing

Use `std::chrono::steady_clock` for elapsed time.  Report percentiles such as
p50, p99, and p99.9; an average can hide rare but important latency spikes.

## Concrete low-latency techniques

1. Use nonblocking sockets so one client cannot stall the event loop.
2. Disable Nagle with `TCP_NODELAY` for tiny request/response TCP messages.
3. Avoid extra copies by reading into reusable buffers and using views safely.
4. Batch syscalls carefully; batching can reduce overhead or add queueing delay.
5. Minimize allocations with preallocated buffers, pools, or arenas when measured.
6. Tune socket buffers for the workload instead of assuming defaults are right.
7. Keep hot paths free of logging, formatting, heap churn, and broad abstractions.
8. Pin important threads only when CPU isolation helps measured jitter.
9. Watch NUMA effects on larger machines; remote memory can add latency.
10. Measure kernel time and application time separately with tools.

## Practical architecture to study

A beginner-friendly low-latency server can look like this:

1. One accept loop accepts TCP connections.
2. N worker event loops each own a shard of sockets.
3. Each connection owns input and output buffers.
4. Messages use a length-prefixed binary protocol.
5. Cross-thread sharing is minimized.
6. Message objects are reused where safe.
7. Backpressure is explicit when output buffers grow.

This teaches socket lifecycle, event loops, buffers, framing, concurrency, and
measurement without requiring advanced kernel-bypass tooling.

## What to build, in order

1. **Blocking TCP echo server/client** — learn socket lifecycle.
2. **Nonblocking echo server with epoll** — learn readiness loops and partial
   reads/writes.
3. **Length-prefixed message protocol** — learn framing and buffering.
4. **Multi-client chat or market-data broadcaster** — learn fanout and
   backpressure.
5. **UDP sequenced feed** — learn loss, sequence numbers, duplicates, and reorder
   handling.
6. **Latency benchmark harness** — measure round trip, p50/p99, message-size
   sensitivity, and the effect of `TCP_NODELAY`.

## Important pitfalls

- Assuming average latency matters more than tail latency.
- Using `std::endl` in hot paths; it flushes the stream.
- Logging synchronously for every packet.
- Parsing with many temporary strings.
- Protecting everything with one mutex.
- Allocating once per packet.
- Assuming UDP is always lower latency in practice.
- Ignoring backpressure.
- Benchmarking only on localhost and trusting the results for real networks.

## Tools and libraries worth knowing

- **Boost.Asio / standalone Asio**: async design patterns in C++.
- **libuv**: event-loop ideas used outside C++ too.
- **io_uring**: modern Linux async I/O, useful after sockets and epoll.
- **DPDK**: advanced kernel-bypass networking for later study.
- **Wireshark**: inspect packets and protocol behavior.
- **perf**: profile CPU and cache behavior.
- **strace**: see syscalls such as `send`, `recv`, and `epoll_wait`.
- **ss / netstat**: inspect socket state and queues.

## Tiny mindset example

Avoid a hot path shaped like this:

```text
read into temporary string
split into vector of strings
allocate message object
push through shared queue
log every packet
```

Prefer a hot path shaped like this:

```text
read into preallocated buffer
parse in place
reuse message storage
keep connection on owning thread
sample logs off hot path
```

If you want to improve quickly: write raw socket code first, benchmark every
version, then read your own flamegraphs and packet traces.
