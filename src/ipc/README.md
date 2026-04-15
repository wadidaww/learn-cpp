# IPC in C++ — Inter-Process Communication

This directory demonstrates the most important IPC mechanisms available on
Linux/POSIX systems, implemented in modern C++ (C++17) with a focus on
**correctness**, **efficiency**, and **best practices**.

---

## Table of Contents

1. [What is IPC?](#what-is-ipc)
2. [Mechanisms Overview](#mechanisms-overview)
3. [Quick Comparison](#quick-comparison)
4. [Directory Structure](#directory-structure)
5. [Build Instructions](#build-instructions)
6. [Running the Examples](#running-the-examples)
   - [Anonymous Pipes](#1-anonymous-pipes)
   - [Named Pipes (FIFO)](#2-named-pipes-fifo)
   - [Shared Memory + Semaphores](#3-shared-memory--posix-semaphores)
   - [POSIX Message Queues](#4-posix-message-queues)
   - [Unix Domain Sockets](#5-unix-domain-sockets)
7. [Best Practices Summary](#best-practices-summary)
8. [When to Use Which Mechanism](#when-to-use-which-mechanism)

---

## What is IPC?

**Inter-Process Communication (IPC)** refers to the mechanisms that allow
separate operating-system processes to exchange data and coordinate their
actions. Unlike threads within the same process (which share an address space),
separate processes have isolated memory — IPC is the bridge between them.

Common use cases include:
- **Microservices / daemon communication** (e.g. `systemd` ↔ service)
- **Shell pipelines** (`ls | grep foo` — anonymous pipes)
- **Shared state without threads** (shared memory)
- **Event notification** (message queues, signals)
- **High-performance local RPC** (Unix Domain Sockets)

---

## Mechanisms Overview

### 1. Anonymous Pipes (`pipe/`)

A **pipe** is a unidirectional, kernel-buffered byte channel between two
related processes (parent ↔ child after `fork()`).

| Property       | Value                                          |
|---------------|------------------------------------------------|
| Direction      | Unidirectional                                 |
| Participants   | Related processes (parent/child/sibling)       |
| Persistence    | None — lives as long as both ends are open     |
| Kernel buffer  | ~64 KB (Linux); `write()` blocks when full     |
| Throughput     | Good for streaming data                        |

**How it works:**
```
Parent                     Child
  │  write(pipe[1], data)    │
  │──────────────────────────►│  read(pipe[0], buf)
  │                           │
```

**When to use:** Simple parent↔child data streaming, shell-style pipelines.

---

### 2. Named Pipes — FIFO (`named_pipe/`)

A **FIFO** (First-In, First-Out) is a special filesystem file that behaves like
a pipe but is accessible by **any** two processes that know its path.

| Property       | Value                                          |
|---------------|------------------------------------------------|
| Direction      | Unidirectional                                 |
| Participants   | Any processes that have filesystem access      |
| Persistence    | Until `unlink()`'d                             |
| Kernel buffer  | Same as anonymous pipes                        |
| Throughput     | Same as anonymous pipes                        |

**Key difference from anonymous pipes:**  
`open()` on a FIFO **blocks** until both ends are opened, providing a natural
rendezvous point between two independent processes.

**When to use:** Streaming data between unrelated processes; logging pipelines.

---

### 3. Shared Memory + POSIX Semaphores (`shared_memory/`)

**Shared memory** maps the same physical memory into multiple process address
spaces. It is the **fastest** IPC mechanism because data transfer happens
purely in memory — the kernel is not involved in the actual copy.

| Property       | Value                                             |
|---------------|---------------------------------------------------|
| Direction      | Bidirectional                                     |
| Participants   | Any processes that know the name                  |
| Persistence    | Until `shm_unlink()` or reboot                    |
| Synchronisation| **None** built-in — you must add semaphores/mutex |
| Throughput     | Highest (zero-copy, memory-speed)                 |

**The producer/consumer semaphore protocol** used here:

```
sem_empty (starts=1) ──►  Writer                  Reader  ◄── sem_full (starts=0)
                          sem_wait(sem_empty)              sem_wait(sem_full)
                          write shared data               read shared data
                          sem_post(sem_full)               sem_post(sem_empty)
```

This guarantees:
- No data race (only one side accesses the buffer at a time)
- No busy-waiting (semaphore blocks until signalled)
- No missed wakeups

**Link flags:** `-lrt -lpthread`

**When to use:** High-throughput data exchange, video/audio processing, shared
lookup tables.

---

### 4. POSIX Message Queues (`message_queue/`)

A **message queue** is a kernel-maintained list of typed messages. Unlike
pipes, it **preserves message boundaries** and supports **priority ordering**.

| Property         | Value                                          |
|-----------------|------------------------------------------------|
| Direction        | Unidirectional (one queue per direction)       |
| Participants     | Any processes that know the queue name         |
| Persistence      | Until `mq_unlink()` or reboot                 |
| Message boundary | Yes — each `mq_send`/`mq_receive` is atomic   |
| Priority         | Yes — highest priority dequeued first          |
| Kernel buffer    | Configurable (`mq_attr.mq_maxmsg`)             |

**Visible in** `/dev/mqueue` (Linux).

**When to use:** Event/notification systems, task queues with urgency levels,
replacing `select()`-based notification fans.

**Link flags:** `-lrt`

---

### 5. Unix Domain Sockets (`unix_socket/`)

**Unix Domain Sockets (UDS)** provide the full BSD socket API (`connect`,
`send`, `recv`) but communicate through the kernel without any network stack.

| Property         | Value                                          |
|-----------------|------------------------------------------------|
| Direction        | **Bidirectional, full-duplex**                 |
| Participants     | Any processes with filesystem access           |
| Persistence      | Socket file removed by server on exit          |
| Modes            | `SOCK_STREAM` (TCP-like) or `SOCK_DGRAM`       |
| Extra features   | Credential passing, FD passing (SCM_RIGHTS)    |
| Throughput       | 2–4× faster than TCP loopback                  |

UDS are the mechanism of choice for **local RPC** — D-Bus, Docker daemon,
PostgreSQL's local connections, and systemd all use UDS internally.

**When to use:** Request/reply protocols between unrelated processes; when
you want bidirectional communication or may later migrate to a network socket.

---

## Quick Comparison

| Mechanism            | Direction  | Unrelated? | Boundaries | Priority | Speed     |
|---------------------|-----------|-----------|-----------|---------|-----------|
| Anonymous Pipe       | Uni        | ✗         | No         | No      | Fast      |
| Named Pipe (FIFO)    | Uni        | ✓         | No         | No      | Fast      |
| Shared Memory        | Bi         | ✓         | No*        | No      | **Fastest** |
| Message Queue        | Uni        | ✓         | **Yes**    | **Yes** | Fast      |
| Unix Domain Socket   | **Bi**     | ✓         | No**       | No      | Very fast |

\* Boundaries must be implemented in the shared data structure.  
\*\* Use `SOCK_DGRAM` UDS for message boundaries.

---

## Directory Structure

```
src/ipc/
├── CMakeLists.txt             ← build targets for all examples
├── README.md                  ← this file
│
├── pipe/
│   └── pipe_example.cpp       ← anonymous pipe between parent and child
│
├── named_pipe/
│   ├── writer.cpp             ← FIFO producer (creates & writes)
│   └── reader.cpp             ← FIFO consumer (reads)
│
├── shared_memory/
│   ├── writer.cpp             ← shared-mem producer + semaphore
│   └── reader.cpp             ← shared-mem consumer + semaphore
│
├── message_queue/
│   ├── sender.cpp             ← POSIX MQ sender (with priorities)
│   └── receiver.cpp           ← POSIX MQ receiver (priority order)
│
└── unix_socket/
    ├── server.cpp             ← UDS server (request/reply)
    └── client.cpp             ← UDS client
```

---

## Build Instructions

```bash
# From the repository root
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- ipc_pipe \
                   ipc_fifo_writer ipc_fifo_reader \
                   ipc_shm_writer  ipc_shm_reader \
                   ipc_mq_sender   ipc_mq_receiver \
                   ipc_uds_server  ipc_uds_client
```

Binaries are placed in `build/src/ipc/`.

**Requirements:**
- C++17 compiler (GCC 7+, Clang 5+)
- Linux (POSIX shared memory, message queues, UDS)
- Link with `-lrt` for shared memory and message queues (handled by CMake)

---

## Running the Examples

### 1. Anonymous Pipes

```bash
./ipc_pipe
```

Single process — the program forks internally.

**Expected output:**
```
[parent] Sending: Hello from parent (msg 1)
[child]  Waiting for messages...
[child]  Received: Hello from parent (msg 1)
...
[parent] Child exited with status 0.
```

---

### 2. Named Pipes (FIFO)

Open two terminals and run from `build/src/ipc/`:

```bash
# Terminal 1 (start first — blocks until writer connects)
./ipc_fifo_reader

# Terminal 2
./ipc_fifo_writer
```

**Expected output (reader):**
```
[reader] Opening FIFO for reading (will block until writer connects)...
[reader] Writer connected. Reading messages...
[reader] Received: FIFO message 1: Hello!
...
[reader] EOF – writer closed. Done.
```

---

### 3. Shared Memory + POSIX Semaphores

```bash
# Terminal 1
./ipc_shm_reader

# Terminal 2
./ipc_shm_writer
```

**Expected output (writer):**
```
[shm_writer] Shared memory and semaphores ready. Writing 5 values...
[shm_writer] Wrote: value=1  msg="Hello from writer, value=1"
...
```

**Expected output (reader):**
```
[shm_reader] Connected. Waiting for 5 values...
[shm_reader] Read: value=1  msg="Hello from writer, value=1"
...
[shm_reader] Done.
```

---

### 4. POSIX Message Queues

```bash
# Terminal 1 (start first — creates the queue)
./ipc_mq_receiver

# Terminal 2
./ipc_mq_sender
```

Notice that the **receiver dequeues messages in priority order** (high → low),
regardless of the order in which they were sent:

```
[mq_sender]   Sent order: prio=1, prio=5, prio=3, prio=5, prio=1
[mq_receiver] Recv order: prio=5, prio=5, prio=3, prio=1, prio=1  ← sorted!
```

---

### 5. Unix Domain Sockets

```bash
# Terminal 1
./ipc_uds_server

# Terminal 2
./ipc_uds_client
```

**Expected output (server):**
```
[uds_server] Listening on /tmp/ipc_demo.sock. Waiting for client...
[uds_server] Client connected.
[uds_server] Request: Hello from UDS client!
[uds_server] Response sent.
[uds_server] Done.
```

**Expected output (client):**
```
[uds_client] Connected to server.
[uds_client] Sent: Hello from UDS client!
[uds_client] Response: Hello from UDS server! Request received and processed.
[uds_client] Done.
```

---

## Best Practices Summary

| Practice | Explanation |
|---------|-------------|
| **Close unused pipe ends** | After `fork()`, close the end you don't use. This ensures the reader gets EOF when the writer exits. |
| **Check every syscall** | `pipe()`, `fork()`, `open()`, `mmap()`, `mq_open()` all fail. Always handle errors. |
| **Unlink from one side only** | For FIFOs, shared memory, and message queues, have one well-defined owner call `unlink()` to avoid double-unlink races. |
| **Use semaphores with shared memory** | Shared memory has no built-in synchronisation. Always protect shared state with a semaphore, mutex, or atomic. |
| **Prefer `SOCK_STREAM` UDS** | For bidirectional communication. Switch to `SOCK_DGRAM` only if you need datagram semantics (message boundaries). |
| **Remove stale socket files** | Call `unlink(path)` before `bind()` to remove socket files left by a previous crash. |
| **Set explicit `mq_attr`** | Don't rely on system defaults for message queue depth and size — set them explicitly to avoid unexpected `EAGAIN`. |
| **Use `O_CLOEXEC`** | When spawning child processes that should not inherit file descriptors, open with `O_CLOEXEC` or call `fcntl(fd, F_SETFD, FD_CLOEXEC)`. |
| **Prefer `POSIX` APIs over `SysV`** | POSIX shared memory (`shm_open`) and message queues (`mq_open`) have a cleaner API and better integration with the filesystem than the older SysV equivalents (`shmget`, `msgget`). |

---

## When to Use Which Mechanism

```
Need to communicate between parent and child?
  └─ Anonymous pipe (simplest)

Need to stream data between unrelated processes?
  └─ Named pipe (FIFO) if one-directional
  └─ Unix Domain Socket (SOCK_STREAM) if bidirectional

Need the highest possible throughput (large data)?
  └─ Shared memory + semaphore

Need message boundaries or priority ordering?
  └─ POSIX message queue

Need a request/reply protocol or FD passing?
  └─ Unix Domain Socket
```
