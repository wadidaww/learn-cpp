# Day 15 · TCP vs UDP, syscall cost, Nagle, and message framing

**Why trading firms care:** orders go to the exchange over TCP (FIX, OUCH, BOE, ETI); market data arrives over UDP multicast. You must know what each protocol costs, why `TCP_NODELAY` is always set, and that TCP is a **byte stream**: one `recv` can return half a message or three messages. Getting framing wrong is the classic gateway bug.

## 1. Concepts (45 min)

### Syscall and kernel networking cost
- A syscall is a user→kernel transition: ~100 ns minimum on modern CPUs, more with Spectre/Meltdown mitigations. A `send` or `recv` through the kernel network stack costs ~1–5 µs each way, and a loopback round trip is typically ~10–30 µs.
- Context switch (thread blocks and another runs): ~2–5 µs plus cold caches afterwards.
- That is why HFT threads **busy-poll** instead of blocking, and why the fastest firms bypass the kernel entirely (day 18).

### TCP vs UDP
| | TCP | UDP |
|---|---|---|
| Model | Reliable ordered **byte stream** | Independent datagrams |
| Loss | Retransmits (adds RTT-scale delay) | Lost is lost; the app detects gaps |
| Order | In order | May reorder |
| Setup | 3-way handshake | None |
| One-to-many | No | Multicast |
| HFT use | Order entry (exchange needs reliability and session) | Market data (one sender, thousands of receivers, latency over reliability) |

### Nagle's algorithm and delayed ACK
- **Nagle:** while unacknowledged data is in flight, TCP holds small writes to merge them into a full segment.
- **Delayed ACK:** the receiver waits up to ~40 ms (Linux) to piggyback the ACK on a reply.
- Together: send a header, then a body in a second `write`, then wait for a reply → the body sits in your kernel waiting for an ACK that the peer is delaying. **~40 ms stall** on a request that should take 20 µs.
- Fix: `setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, 1)` on every latency-sensitive socket, and build each message in one buffer and send it with one call (or `writev`/`sendmsg` for scatter/gather).
- Related: `TCP_QUICKACK` (disable delayed ACK for a while), `SO_SNDBUF`/`SO_RCVBUF` sizes, `SO_BUSY_POLL`.

### Framing a byte stream
TCP gives you no message boundaries. Every protocol defines them: SoupBinTCP and many binary protocols use a **2-byte big-endian length prefix**; FIX uses `9=BodyLength` and the `10=CheckSum` trailer. The receiver must:
1. Append whatever `recv` returned to a buffer.
2. While the buffer holds a complete frame (length header + that many bytes): hand out the payload, advance.
3. Keep the partial remainder for the next `recv` (compact it to the front, or use a ring).
4. Reject impossible lengths (a corrupt or hostile peer must not make you allocate or overflow).

### The socket calls you should know by heart
`socket`, `bind`, `listen`, `accept`, `connect`, `send`/`recv` (with `MSG_DONTWAIT`, `MSG_NOSIGNAL`), `setsockopt`, `fcntl(O_NONBLOCK)`, `close`, `shutdown`. `recv` returning 0 means the peer closed; −1 with `EAGAIN` means no data (non-blocking); `EINTR` means retry. `send` may write fewer bytes than asked: loop. Writing to a closed socket raises `SIGPIPE` unless you pass `MSG_NOSIGNAL`.

## 2. Exercise (2 h)

In `solution.hpp`:
1. `FrameDecoder<Cap>`: `feed(bytes)` appends received bytes (returns false if they do not fit), `next(payload)` returns the next complete payload as a span into the internal buffer (valid until the next `feed`), `consume()` releases it. Frames are `u16 big-endian length + payload`. A length above `Cap - 2` puts the decoder in an error state.
2. `encode_frame(payload, out)`: writes the header and payload into `out`, returns bytes written or 0.
3. Socket helpers: `tcp_listen(port, bound_port)` (port 0 = let the kernel pick, report it back), `tcp_accept`, `tcp_connect(port)` (127.0.0.1), `set_nodelay(fd, on)`, `send_all(fd, data)` (loops on partial writes, uses `MSG_NOSIGNAL`), `recv_some(fd, buf)`.

The tests feed the decoder frames split at every possible byte boundary, then run a real loopback echo through your socket helpers.

Then run `bench`: it measures loopback round trips with a header+body sent as **two writes**, with and without `TCP_NODELAY`. You should see the Nagle/delayed-ACK stall with Nagle on. Explain it.

**Done when:** tests pass under ASan, and you can explain the bench output.

## 3. Say it out loud

<details><summary>1. Why is market data UDP and order entry TCP?</summary>
Market data is one-to-many and latency-critical: multicast UDP lets the exchange send one packet to every subscriber, and a lost packet is recovered from a retransmission or snapshot channel instead of stalling the stream. Orders are one-to-one and must be reliable and ordered, with a session (sequence numbers, logon), so TCP.
</details>

<details><summary>2. What is Nagle's algorithm and why disable it?</summary>
It delays small writes while earlier data is unacknowledged, to merge them. Combined with delayed ACKs on the peer it can add ~40 ms to a request. Low-latency sockets set TCP_NODELAY and send each message in one call.
</details>

<details><summary>3. recv returned 100 bytes. How many messages is that?</summary>
Unknown: TCP is a byte stream. It may be part of a message or several. Append to a buffer, parse complete frames by their length header, keep the remainder.
</details>

<details><summary>4. What does a syscall cost and why avoid it?</summary>
A mode switch costs ~100 ns or more, plus kernel work (a network send or receive is µs), plus cache and TLB pollution. Hot loops avoid syscalls by busy-polling, batching, and kernel bypass.
</details>

<details><summary>5. send() returned less than you asked. Why, and what do you do?</summary>
The socket send buffer was nearly full (or a signal interrupted it). Loop until everything is sent, or with non-blocking sockets keep the rest in a user-space buffer and wait for writability (EPOLLOUT).
</details>

<details><summary>6. What does recv returning 0 mean?</summary>
The peer closed its side (orderly shutdown). -1 with EAGAIN/EWOULDBLOCK means no data on a non-blocking socket; EINTR means retry.
</details>
