# IPC with File Descriptors — `select()` and `epoll()`

This directory is a **separate companion** to `src/ipc/` and focuses on two
inter-related topics that underpin all fd-based IPC:

1. **File-descriptor passing** — transferring an *open* file descriptor from
   one process to a completely unrelated process using `sendmsg()` / `recvmsg()`
   with `SCM_RIGHTS` ancillary data.

2. **fd-multiplexing** — monitoring many pipe (or socket) file descriptors
   simultaneously without spawning a thread per source, using the POSIX
   `select()` API and the Linux-specific (O(1)) `epoll()` API.

---

## Table of Contents

1. [Background: What is a File Descriptor?](#background-what-is-a-file-descriptor)
2. [FD Passing — SCM\_RIGHTS](#fd-passing--scm_rights)
3. [fd-Multiplexing: select() vs epoll()](#fd-multiplexing-select-vs-epoll)
   - [select()](#select)
   - [epoll()](#epoll)
   - [Level-triggered vs Edge-triggered](#level-triggered-vs-edge-triggered)
4. [Comparison Table](#comparison-table)
5. [Directory Structure](#directory-structure)
6. [Build Instructions](#build-instructions)
7. [Running the Examples](#running-the-examples)
   - [FD Passing](#1-fd-passing)
   - [select() IPC](#2-select-ipc)
   - [epoll() IPC](#3-epoll-ipc)
8. [Best Practices Summary](#best-practices-summary)

---

## Background: What is a File Descriptor?

A **file descriptor** (fd) is a small non-negative integer that a process uses
to refer to an open-file description maintained by the kernel.  That description
stores:

- A reference to the underlying inode (file, pipe, socket, …)
- The current file-position pointer
- The access mode (read-only, write-only, read-write)
- Status flags (`O_NONBLOCK`, `O_APPEND`, …)

Every process has its own *descriptor table* (fd 0 = stdin, fd 1 = stdout, fd 2
= stderr, then whatever `open()`/`pipe()`/`socket()` returns).  Two descriptors
in *different* processes can refer to the **same** open-file description — the
kernel keeps a reference count.  This is exactly what `SCM_RIGHTS` exploits.

---

## FD Passing — SCM\_RIGHTS

### The problem it solves

Sometimes a **privileged process** (or a process that already has the FD open)
needs to hand an open resource to a **sandboxed / unprivileged process** that
cannot open the resource itself:

- A privileged helper opens `/etc/shadow` and passes the FD to a worker.
- `systemd` passes a pre-bound listen socket to a service it starts.
- A media encoder passes a `memfd` (memory-backed file) FD to avoid copying.

### How it works

`sendmsg()` supports *ancillary data* — metadata attached to a socket message
alongside ordinary bytes.  A `SOL_SOCKET` / `SCM_RIGHTS` ancillary message
embeds an array of file descriptors.  When the kernel delivers the message via
`recvmsg()`, it creates *duplicate* descriptors in the receiver's table that
refer to the same open-file descriptions.

```
Sender                                 Receiver
  ┌──────────┐   sendmsg()+SCM_RIGHTS   ┌──────────┐
  │ fd=3 ──►│──────────────────────────►│ fd=5     │
  │ (file)   │   Unix Domain Socket     │ (same    │
  └──────────┘                          │  file)   │
                                        └──────────┘
```

**Key macros** (never compute offsets manually — alignment rules are
platform-specific):

| Macro | Purpose |
|---|---|
| `CMSG_SPACE(n)` | Total buffer size for n bytes of ancillary payload |
| `CMSG_LEN(n)` | Value to store in `cmsg_len` |
| `CMSG_FIRSTHDR(msghdr*)` | Pointer to first `cmsghdr` in control buffer |
| `CMSG_DATA(cmsghdr*)` | Pointer to ancillary payload (where FDs live) |

### Example flow (`fd_passing/`)

```
fd_receiver (server)                 fd_sender (client)
─────────────────────────────────────────────────────────
socket() + bind() + listen()
accept() [blocks]
                                     open(/tmp/…) → fd=3
                                     socket() + connect()
recvmsg() ← ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ sendmsg(SCM_RIGHTS fd=3)
extract fd=5 from cmsg
read(fd=5) → file content
close(fd=5)
```

---

## fd-Multiplexing: select() vs epoll()

### The problem it solves

A process that reads from *N* independent data sources (children via pipes, TCP
clients, FIFO writers) has two naive options:

1. **One blocking `read()` per source** — you process source 0 while sources
   1…N-1 starve.
2. **One thread per source** — wastes memory and incurs context-switch overhead.

The correct solution is to ask the kernel *"which of these FDs has data right
now?"* and only then read from the ready ones.

---

### select()

```c
int select(int nfds,
           fd_set *readfds,    // watch for readability
           fd_set *writefds,   // watch for writability
           fd_set *exceptfds,  // watch for exceptions (OOB data)
           struct timeval *timeout); // NULL = block forever; {0,0} = poll
```

**How it works:**
- You populate bit-arrays (`fd_set`) using `FD_ZERO` / `FD_SET`.
- The kernel scans all FDs from 0 to `nfds-1` and checks their status.
- On return the kernel *modifies* the sets in-place — only ready bits remain.
- You call `FD_ISSET(fd, &set)` to check which FDs are ready.

**Limitations:**
- `FD_SETSIZE` = 1024 on most systems — cannot watch more than 1024 FDs.
- O(n) scan inside the kernel on every call.
- You must **re-build the fd_set before every call** (select() clobbers it).
- `nfds` must be `max_fd + 1` — easy to get wrong.

---

### epoll()

epoll is a Linux-specific, scalable alternative introduced in kernel 2.5.44.

```c
// 1. Create an epoll instance (returns a fd itself)
int epfd = epoll_create1(EPOLL_CLOEXEC);

// 2. Register a file descriptor to watch
struct epoll_event ev = { .events = EPOLLIN, .data.fd = pipe_fd };
epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_fd, &ev);

// 3. Wait for events
struct epoll_event events[MAX];
int n = epoll_wait(epfd, events, MAX, timeout_ms);
// events[0..n-1] contain the ready descriptors
```

**How it differs from select():**
- The kernel maintains a persistent interest list — no re-registration needed.
- Only ready events are returned — O(1) per event, regardless of watch-list size.
- No artificial FD limit.
- `epoll_event.data` carries arbitrary user data (fd, pointer, integer) —
  no need to scan an array to find which FD fired.

---

### Level-triggered vs Edge-triggered

Both select() and epoll's default mode are **level-triggered (LT)**:

> The kernel notifies you as long as there is data available to read.

`epoll` also supports **edge-triggered (ET)** mode (`EPOLLIN | EPOLLET`):

> The kernel notifies you **once** when the FD transitions from not-ready to
> ready. After that, no more notifications until you drain all data AND the FD
> becomes ready again.

| | Level-triggered | Edge-triggered |
|---|---|---|
| Notification | Repeated while data available | Once per transition |
| Required | Normal `read()` | Loop `read()` until `EAGAIN` |
| FD mode | Blocking OK | **Must be O_NONBLOCK** |
| Missed events | No | Yes, if you don't drain fully |
| Common use | General purpose | High-performance servers |

The `epoll_example.cpp` uses ET mode and demonstrates the "drain until EAGAIN"
pattern correctly.

---

## Comparison Table

| API | FD limit | Complexity | Level/Edge | Platform |
|-----|---------|-----------|-----------|---------|
| `select()` | 1024 | O(n) scan | Level only | POSIX |
| `poll()` | unlimited | O(n) scan | Level only | POSIX |
| `epoll()` | unlimited | O(1)/event | Both | Linux |
| `kqueue()` | unlimited | O(1)/event | Both | BSD/macOS |

---

## Directory Structure

```
src/ipc_fd/
├── CMakeLists.txt           ← build targets for all examples
├── README.md                ← this file
│
├── fd_passing/
│   ├── sender.cpp           ← creates a file, passes its FD via SCM_RIGHTS
│   └── receiver.cpp         ← receives the FD, reads file without knowing path
│
├── select_ipc/
│   └── select_example.cpp   ← parent monitors 4 pipe FDs with select()
│
└── epoll_ipc/
    └── epoll_example.cpp    ← parent monitors 4 pipe FDs with epoll (EPOLLET)
```

---

## Build Instructions

```bash
# From the repository root
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- ipc_fd_sender ipc_fd_receiver ipc_select ipc_epoll
```

Binaries are placed in `build/src/ipc_fd/`.

**Requirements:**
- C++17 compiler (GCC 7+, Clang 5+)
- Linux (epoll is Linux-specific; select/FD-passing are POSIX)

---

## Running the Examples

### 1. FD Passing

Run in two terminals from `build/src/ipc_fd/`:

```bash
# Terminal 1 — receiver (server side, start first)
./ipc_fd_receiver

# Terminal 2 — sender (client side)
./ipc_fd_sender
```

**Expected output (receiver):**
```
[fd_receiver] Listening on /tmp/ipc_fd_demo.sock. Waiting for sender...
[fd_receiver] Sender connected.
[fd_receiver] Data frame received: "FD_TRANSFER"
[fd_receiver] Received file descriptor: fd=5
[fd_receiver] Reading file content via received fd:
──────────────────────────────────────────────────
This content was written by fd_sender.
The fd_receiver will read it via an fd passed over a Unix socket,
without knowing the file path at all.
──────────────────────────────────────────────────
[fd_receiver] Done.
```

Notice that `fd_receiver` never calls `open()` with a path — it reads the file
purely through the descriptor it received.

---

### 2. select() IPC

```bash
./ipc_select
```

A single program; it forks children internally.

**Expected output:**
```
[select] Parent waiting for 4 messages via select()...

[select] [msg 1/4] fd=9 | Hello from child 3 (delay=150 ms)
[select] [msg 2/4] fd=7 | Hello from child 2 (delay=300 ms)
[select] [msg 3/4] fd=5 | Hello from child 1 (delay=450 ms)
[select] [msg 4/4] fd=3 | Hello from child 0 (delay=600 ms)

[select] All 4 messages received. Done.
```

Children are created with different delays, so `select()` processes whichever
becomes ready first — observe the reverse order (child 3 arrives first).

---

### 3. epoll() IPC

```bash
./ipc_epoll
```

A single program; it forks children internally.

**Expected output:**
```
[epoll] Registered 4 pipe read-ends with epoll (EPOLLIN|EPOLLET).

[epoll] Parent event loop started. Waiting for children...

[epoll] [event 1] fd=9 child=3 | Hello from child 3 (delay=200 ms)
[epoll] [event 2] fd=7 child=2 | Hello from child 2 (delay=400 ms)
[epoll] [event 3] fd=5 child=1 | Hello from child 1 (delay=600 ms)
[epoll] [event 4] fd=3 child=0 | Hello from child 0 (delay=800 ms)

[epoll] All 4 messages received. Done.
```

Same ordering as select, but using epoll's O(1) event delivery.  The
`epoll_event.data.u32` field carries the child index so the parent can
identify which pipe fired without scanning an array.

---

## Best Practices Summary

| Practice | Explanation |
|---------|-------------|
| **Use CMSG\_SPACE / CMSG\_DATA macros** | Never compute ancillary-data offsets by hand; alignment rules differ by platform. |
| **Re-build fd\_set before every select()** | select() overwrites the sets in-place; a stale set from the last iteration will give wrong results. |
| **Set nfds = max\_fd + 1** | The kernel only checks 0..nfds-1; if nfds is too small, high-numbered FDs are silently ignored. |
| **Use epoll\_create1(EPOLL\_CLOEXEC)** | Prevents the epoll FD from leaking into child processes created with exec(). |
| **O\_NONBLOCK required for EPOLLET** | Edge-triggered epoll delivers one notification per transition; without O_NONBLOCK, a blocking read could deadlock if the kernel reports ready-but-partial. |
| **Drain until EAGAIN in ET mode** | After an EPOLLET notification, loop read() until errno == EAGAIN; missing bytes won't trigger another event. |
| **Check EPOLLIN before EPOLLHUP** | EPOLLIN and EPOLLHUP can arrive together (write + close); read the data first, then close the FD when read() returns 0. |
| **EPOLL\_CTL\_DEL before close()** | Technically the kernel auto-removes a closed FD, but explicit removal avoids confusion when the same FD number is reused. |
