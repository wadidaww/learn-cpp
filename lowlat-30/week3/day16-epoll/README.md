# Day 16 · Non-blocking I/O and epoll (level vs edge triggered), busy polling

**Why trading firms care:** a gateway or a market-data recovery service handles many sockets on one thread. You must know the event-loop pattern, the difference between level- and edge-triggered epoll, and why a latency-critical loop calls `epoll_wait(..., 0)` (spin) instead of blocking.

## 1. Concepts (45 min)

### Blocking vs non-blocking
- Blocking `recv` puts the thread to sleep until data arrives: a context switch out and a wake-up later (µs, plus cold caches).
- Non-blocking (`fcntl(fd, F_SETFL, O_NONBLOCK)` or `SOCK_NONBLOCK`): `recv` returns `-1/EAGAIN` immediately if nothing is there.
- With many sockets you need to know **which** are ready: `select` (fd limit 1024, O(n) scan every call), `poll` (no limit, still O(n)), **`epoll`** (O(ready) per call, the kernel keeps the interest list), `io_uring` (submission/completion rings shared with the kernel, can avoid syscalls with SQPOLL).

### epoll API
```cpp
int ep = epoll_create1(EPOLL_CLOEXEC);
epoll_event ev{.events = EPOLLIN | EPOLLET, .data = {.fd = fd}};
epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev);
epoll_event evs[64];
int n = epoll_wait(ep, evs, 64, timeout_ms);   // -1 block, 0 return at once (busy poll), >0 ms
```
`data` can hold a pointer or index to your connection object instead of the fd, which saves a lookup.

### Level-triggered vs edge-triggered
| | Level-triggered (default) | Edge-triggered (`EPOLLET`) |
|---|---|---|
| Reports | Every call while the fd is readable | Only when new data arrives (state change) |
| Your loop | Can read once per event | **Must** read until `EAGAIN`, or the rest of the data is never reported again |
| Cost | More wake-ups | Fewer epoll events, more care |
The classic ET bug: read one chunk, return to `epoll_wait`, and hang forever with data still in the socket.

### Writing
If `send` returns `EAGAIN` or a partial count, the kernel buffer is full. Keep the remainder in your own buffer, register `EPOLLOUT`, and flush when writable. Remove `EPOLLOUT` once empty, or (level-triggered) it fires constantly.

### Busy polling
- `epoll_wait(ep, evs, n, 0)` in a loop never sleeps: no wake-up latency, but the core runs at 100%. Pinned latency-critical threads do exactly this.
- Kernel-side: `SO_BUSY_POLL` / `net.core.busy_poll` make the kernel spin on the NIC queue for a while instead of waiting for an interrupt.
- With kernel bypass (day 18) you poll the NIC ring directly from user space and epoll disappears.

### The reactor pattern
One thread, one `epoll` set, a loop: wait → for each event: accept / read until EAGAIN / process complete messages / write or queue output. Slow work goes to other threads via SPSC rings (week 1), never inside the loop.

## 2. Exercise (2 h): `ll::EpollEchoServer`

In `solution.hpp`, a single-threaded **edge-triggered** echo server:
- `listen(port, bound_port)`: non-blocking listening socket on 127.0.0.1, registered with epoll.
- `poll(timeout_ms)`: one `epoll_wait`; accept all pending connections (loop until `EAGAIN`), read each readable socket until `EAGAIN`, echo every byte back. If the kernel will not take all the bytes, keep the rest per connection and use `EPOLLOUT`. Close and forget connections on EOF or error. Return the number of events handled, or -1 on error.
- `clients()`: number of open client connections.
- The destructor closes everything.

The tests run your server on one thread, connect 20 clients from another, and check that every byte (including large bursts that force partial writes) comes back.

Then run `bench` and compare round-trip latency with `poll(0)` busy polling vs `poll(-1)` blocking.

**Done when:** tests pass, and you can explain why your read loop must continue until `EAGAIN`.

## 3. Say it out loud

<details><summary>1. select vs poll vs epoll?</summary>
select: fd bitmaps, 1024 limit, the kernel scans all fds each call. poll: array, no limit, still O(n) per call. epoll: the kernel keeps the interest set and a ready list, so each wait is O(ready fds). epoll is the Linux standard for many sockets.
</details>

<details><summary>2. Level- vs edge-triggered?</summary>
Level-triggered keeps reporting a readable fd until you drain it. Edge-triggered reports only transitions, so you must read (and accept) until EAGAIN every time, or you lose the notification for data already buffered.
</details>

<details><summary>3. What do you do when send returns EAGAIN?</summary>
Buffer the unsent bytes for that connection, enable EPOLLOUT, and flush when the socket becomes writable; disable EPOLLOUT when the buffer is empty. Apply backpressure if the buffer grows too large.
</details>

<details><summary>4. Why would a trading system call epoll_wait with timeout 0?</summary>
To never sleep: a blocking wait costs a context switch and wake-up of several µs. A pinned core spinning on epoll_wait(0) (or directly on a kernel-bypass ring) reacts in nanoseconds to microseconds, at the cost of burning that core.
</details>

<details><summary>5. What is io_uring and why is it interesting?</summary>
Shared submission and completion rings between user space and the kernel. You batch many I/O operations per syscall, and with SQPOLL a kernel thread polls the ring so the application can submit with no syscalls at all.
</details>
