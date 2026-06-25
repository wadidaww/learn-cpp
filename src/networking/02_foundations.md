# 02 — Foundations for Low-Latency C++ Networking

Learn these topics in order.  Each layer explains why the next layer matters.

## 1. TCP/IP basics

Know these terms before tuning code:

- **IP address**: identifies a host interface.
- **Port**: identifies an application endpoint on a host.
- **MTU**: maximum packet size before fragmentation on a link.  Ethernet is
  commonly 1500 bytes, but do not hard-code that for every network.
- **Packet loss**: packets can disappear; reliable protocols retransmit.
- **Retransmission**: resending lost data costs at least another network round
  trip.
- **Congestion control**: TCP slows down when the network appears overloaded.
- **Latency vs throughput**: latency is how long one unit takes; throughput is
  how many units per second complete.

### TCP vs UDP intuition

| TCP | UDP |
|-----|-----|
| Reliable ordered byte stream. | Unreliable datagrams/messages. |
| Built-in retransmission and backpressure. | You design loss, reorder, and duplicate handling. |
| Can suffer head-of-line blocking. | No stream head-of-line blocking between datagrams. |
| Great default for RPC and correctness-first services. | Common for market data, games, telemetry, and custom feeds. |

### Nagle's algorithm

Nagle's algorithm combines small TCP writes to improve throughput.  That can add
latency for request/response messages.  `TCP_NODELAY` disables it when tiny
messages must be sent immediately.

```cpp
// TCP example: call this after socket/connect/accept when small writes should
// not wait for Nagle batching.
// int one = 1;
// setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
```

## 2. POSIX/Berkeley sockets

C++ networking on Linux usually begins with the C socket API.  Wrap file
descriptors with RAII in C++, but learn the raw lifecycle first.

Core calls:

- `socket` creates an endpoint.
- `bind` attaches a local IP/port.
- `listen` marks a TCP socket as a passive listener.
- `accept` returns a connected TCP socket for one client.
- `connect` starts a client connection or sets a UDP peer.
- `send` / `recv` transfer bytes on connected sockets.
- `sendmsg` / `recvmsg` support scatter/gather I/O and ancillary data.
- `setsockopt` tunes behavior such as `TCP_NODELAY` and socket buffers.
- `fcntl` enables nonblocking mode with `O_NONBLOCK`.
- `epoll` waits for readiness across many file descriptors on Linux.

You should be able to write, in this order:

1. A blocking TCP echo server and client.
2. A nonblocking TCP client.
3. An event-driven multi-connection server with `epoll`.

## 3. Event-driven I/O

A one-thread-per-connection server is easy to understand but creates scheduling,
stack-memory, and context-switch overhead.  Low-latency servers usually keep many
connections on a small number of event-loop threads.

Concepts:

- `select` and `poll`: portable readiness APIs; useful conceptually.
- `epoll`: Linux readiness API that scales better for many descriptors.
- **Readiness-based loop**: the kernel tells you a socket is probably readable or
  writable, then you drain work until it would block.
- **Level-triggered**: the event repeats while the condition remains true.
- **Edge-triggered**: the event fires when state changes; you must drain until
  `EAGAIN` or risk sleeping while data waits.

## 4. Buffer management

Buffering decides whether your parser is predictable or allocation-heavy.

Learn:

- **Ring buffers**: fixed storage with read/write cursors that wrap.
- **Read/write cursors**: indexes that track consumed and produced bytes.
- **Framing protocols**: rules for finding complete messages in a byte stream.
- **Scatter/gather I/O**: fill or send multiple buffers with one syscall.
- **In-place parsing**: use views into a stable buffer when safe.

Avoid repeated `std::string` growth in packet paths.  Reserve or reuse buffers
when message size bounds are known.

## 5. Serialization

Serialization changes in-memory objects into bytes on the wire.

| Format | Strength | Cost |
|--------|----------|------|
| Text | Easy to debug and type by hand. | Parsing, formatting, and larger packets. |
| Binary | Compact and fast to parse. | Harder debugging and versioning. |
| Fixed-width fields | Simple offsets and predictable CPU. | Less flexible. |
| Varints | Small numbers use fewer bytes. | Branchier parsing. |

Always define byte order.  Network byte order is big-endian; use conversions such
as `htons`, `htonl`, `ntohs`, and `ntohl` at protocol boundaries.

## 6. Concurrency model

Low-latency concurrency is about ownership more than thread count.

Common designs:

- Single-threaded event loop: simplest and often fastest for moderate load.
- One event loop per core: scales by sharding sockets or keys.
- Handoff queues: useful at boundaries, but each handoff adds latency.
- Lock-free queues: useful only when they solve a measured problem.

Prefer:

- CPU pinning for important loops after measurement.
- Sharding by connection, symbol, game room, account, or key.
- Minimal shared mutable state.
- Clear backpressure instead of unbounded queues.
