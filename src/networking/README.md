# C++ Networking and Low Latency Notes

Networking code moves bytes between processes, usually with sockets.  A socket is
an operating-system object that represents one endpoint of a connection or packet
flow.  C++ does not standardize sockets yet, so portable production code often
uses a library such as Boost.Asio, but the POSIX socket API is the best place to
learn what the operating system is doing.

## TCP vs UDP

| Protocol | Use it when | Low-latency notes |
|----------|-------------|-------------------|
| TCP | You need reliable ordered bytes, back-pressure, and congestion control. | Disable Nagle with `TCP_NODELAY` for request/response traffic, avoid tiny writes, and keep connections open. |
| UDP | You prefer messages/datagrams and can handle drops, duplicates, or reordering yourself. | No connection handshake and no head-of-line blocking, so it is common for market data, telemetry, games, and custom protocols. |

## What makes networking low latency?

Low latency is about reducing time spent before a packet is useful to your
program.  The most important habits are:

1. **Measure first.** Use percentiles such as p50/p99/p99.9, not only averages.
2. **Avoid work in the hot path.** Do not allocate memory, parse strings, log, or
   take locks for every packet unless you have measured that it is acceptable.
3. **Use fixed-size binary messages.** They avoid dynamic allocation and expensive
   serialization.  Add version fields if the protocol will evolve.
4. **Batch carefully.** Batching improves throughput but can increase latency if
   a packet waits too long for a batch to fill.
5. **Tune sockets deliberately.** Buffer sizes, `TCP_NODELAY`, non-blocking I/O,
   and event APIs such as `epoll` matter, but each option is workload-specific.
6. **Control scheduling noise.** CPU affinity, real-time priorities, busy polling,
   and isolated CPUs can help in specialized systems, but they trade CPU for
   lower tail latency.
7. **Keep the network simple.** Fewer hops, less congestion, and colocated
   services usually beat clever code.

## Example in this directory

`low_latency_udp.cpp` is a commented, runnable UDP ping-pong benchmark.  It runs a
local echo server in one thread, sends fixed-size packets from a client socket,
and reports round-trip latency percentiles.

Build only this lesson from the repository root:

```bash
g++ -std=c++20 -O2 -Wall -Wextra src/networking/low_latency_udp.cpp -pthread -o /tmp/low_latency_udp
/tmp/low_latency_udp
```

Or build it through CMake as `networking_low_latency_udp` when the top-level build
environment has all optional dependencies installed.

## Reading the code

Focus on the comments marked **low-latency lesson**.  They call out why the code
uses:

- UDP datagrams instead of streams for the demo.
- Fixed-size packet structs.
- Preallocated vectors.
- Non-blocking sockets.
- Socket buffer sizing.
- A bounded spin loop while waiting for replies.
- Percentile reporting instead of only average latency.
