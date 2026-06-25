# 01 — Where Networking Latency Comes From

Low-latency networking is mostly the art of removing avoidable waiting and work
from the path between packet arrival and useful application action.  This file is
a checklist of common latency costs to look for before reaching for advanced
techniques such as kernel bypass.

## Kernel/user transitions

Every `send`, `recv`, `sendmsg`, `recvmsg`, `accept`, or `epoll_wait` crosses
between user space and the kernel.  That transition is not free: registers and
CPU state must be saved, kernel code runs, and the CPU may lose useful cache
state.

Practical guidance:

- Avoid one syscall per tiny piece of work when a single syscall can move the
  same logical message.
- Do not blindly batch everything: batching lowers syscall overhead but can add
  queueing delay while the batch fills.
- Use readiness APIs such as `epoll` so a thread wakes when useful work is
  likely available.

```cpp
// Mindset example only: one recvmsg can receive into multiple buffers.
// That can avoid copying header bytes into one temporary buffer and payload
// bytes into another.
//
// iovec parts[] = {{header.data(), header.size()}, {body.data(), body.size()}};
// msghdr msg{};
// msg.msg_iov = parts;
// msg.msg_iovlen = 2;
// recvmsg(fd, &msg, 0);
```

## Copies

A copy is often hidden behind a friendly abstraction.  Data can be copied by the
NIC, the kernel, socket buffers, protocol parsers, `std::string` growth, queue
handoffs, and serialization layers.

Practical guidance:

- Read into reusable buffers owned by the connection or event loop.
- Parse in place with `std::span` or `std::string_view` when lifetime is clear.
- Prefer scatter/gather I/O (`readv`, `writev`, `sendmsg`, `recvmsg`) when a
  message naturally has multiple contiguous pieces.
- Avoid converting bytes to text and back in the hot path.

## Allocations

Heap allocation can be fast on average and still terrible for tail latency.  It
may take locks, touch cold memory, trigger page faults, or fragment memory.

Bad hot-path pattern:

```cpp
// Each packet may allocate a new string and grow it repeatedly.
std::string message;
message += header_text;
message += payload_text;
```

Better hot-path pattern:

```cpp
// Reuse storage. The exact size and lifetime are explicit.
std::array<std::byte, 1500> packet_buffer{}; // one Ethernet MTU-sized buffer
```

Practical guidance:

- Allocate per connection or per event-loop thread, not per packet.
- Use fixed-size buffers for known maximum message sizes.
- Use arenas or object pools only after measuring; they add complexity.

## Locks and contention

A lock protecting shared state can turn many CPU cores into one slow critical
section.  Even lock-free structures can be slower than ownership-based design if
they cause cache-line bouncing.

Practical guidance:

- Prefer one event loop owning a connection from read to write.
- Shard by connection, account, symbol, game room, or key.
- Use message passing for cross-thread handoff, and keep handoff frequency low.
- Keep metrics and logging off the hot path or sample them.

## Context switches

A context switch happens when the OS stops one thread and runs another.  It adds
scheduler overhead and usually damages cache locality.

Practical guidance:

- Avoid one-thread-per-connection servers for large connection counts.
- Use nonblocking sockets and event loops.
- Pin important event-loop threads to CPUs only after you understand the machine;
  pinning can help jitter, but bad pinning can make performance worse.

## Cache misses

Modern CPUs are much faster than memory.  Pointer-heavy data structures may spend
more time waiting for memory than processing packets.

Practical guidance:

- Prefer contiguous storage: `std::array`, `std::vector`, flat structs, and
  fixed-size ring buffers.
- Avoid linked lists and map-heavy hot paths unless measurement supports them.
- Keep frequently accessed fields together and cold/debug fields elsewhere.

## Delayed packet handling

Packets can wait in many queues: NIC queues, kernel socket buffers, application
queues, logging queues, worker queues, or batches.  The code may look fast while
packets spend time waiting before the code sees them.

Practical guidance:

- Measure time at multiple points: before send, after receive, after parse, after
  business logic, and before response.
- Track queue depth and drops.
- Keep backpressure explicit; ignoring it often turns latency spikes into memory
  growth and eventually packet loss.

## Unnecessary protocol overhead

HTTP, JSON, TLS, compression, and schema-flexible formats are useful, but they
cost bytes and CPU.  Low-latency systems often use simpler binary protocols for
hot internal paths.

Practical guidance:

- Use text protocols for learning, debugging, or low-rate control planes.
- Use binary protocols for high-rate, latency-sensitive data planes.
- Keep fixed-width fields when possible; use endian conversion at boundaries.
- Add version fields so the protocol can evolve without expensive guessing.
