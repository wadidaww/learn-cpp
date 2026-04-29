# Boost Library for High Performance Computing in C++

## What is Boost?

[Boost](https://www.boost.org/) is a collection of peer-reviewed, portable C++ source libraries that extend the standard library. Many Boost components have been adopted into the C++ Standard (e.g., `std::thread`, `std::shared_ptr`, `std::regex`). For **High Performance Computing (HPC)**, Boost offers several powerful libraries that enable:

- Asynchronous I/O and networking
- Fine-grained multi-threading and synchronization
- Lock-free data structures
- Distributed-memory parallelism (MPI)
- GPU/OpenCL computing
- Lightweight, cooperative concurrency (fibers/coroutines)

---

## Installation

### Ubuntu / Debian
```bash
sudo apt-get install libboost-all-dev
```

### macOS (Homebrew)
```bash
brew install boost
```

### Building from Source
```bash
./bootstrap.sh --prefix=/usr/local
./b2 install
```

---

## Boost Components Relevant to HPC

### 1. Boost.Asio — Asynchronous I/O (`boost_asio_async.cpp`)

Boost.Asio provides a consistent asynchronous model for network I/O, timers, and serial ports. It is the backbone of many high-performance servers and is the basis for `std::execution` in future C++ standards.

**Key concepts:**
- `io_context` — the event loop / executor
- `async_read` / `async_write` — non-blocking I/O
- `strand` — serialised handler execution without explicit locking
- Thread pools via `io_context` run on multiple threads

**Compile:**
```bash
g++ -std=c++17 boost_asio_async.cpp -lboost_system -pthread -o boost_asio_async
```

---

### 2. Boost.Thread — Advanced Threading (`boost_thread.cpp`)

Boost.Thread predates `<thread>` and still offers extras such as:
- `thread_group` — manage a collection of threads
- `shared_mutex` / `shared_lock` — reader-writer locks
- `future` / `promise` with richer cancellation support
- Barrier and latch primitives

**Compile:**
```bash
g++ -std=c++17 boost_thread.cpp -lboost_thread -lboost_system -pthread -o boost_thread
```

---

### 3. Boost.Lockfree — Lock-Free Data Structures (`boost_lockfree.cpp`)

Lock-free queues and stacks allow multiple threads to communicate without mutexes, dramatically reducing contention in producer-consumer workloads.

**Key types:**
- `boost::lockfree::queue<T>` — MPMC bounded/unbounded queue
- `boost::lockfree::stack<T>` — MPMC stack
- `boost::lockfree::spsc_queue<T>` — single-producer / single-consumer ring buffer (fastest)

**Compile:**
```bash
g++ -std=c++17 boost_lockfree.cpp -pthread -o boost_lockfree
```
*(header-only, no link flags needed beyond pthreads)*

---

### 4. Boost.MPI — Distributed Computing (`boost_mpi.cpp`)

Boost.MPI is a C++ wrapper around the MPI (Message Passing Interface) standard. It leverages Boost.Serialization to send arbitrary C++ objects across nodes, making distributed HPC clusters much easier to program.

**Key features:**
- `mpi::environment` / `mpi::communicator`
- `send` / `recv` with any serializable type
- Collective operations: `broadcast`, `reduce`, `gather`, `scatter`, `all_reduce`
- Non-blocking (`isend` / `irecv`) and communicator groups

**Compile:**
```bash
mpic++ -std=c++17 boost_mpi.cpp -lboost_mpi -lboost_serialization -o boost_mpi
mpirun -n 4 ./boost_mpi
```

---

### 5. Boost.Compute — GPU / OpenCL Computing (`boost_compute.cpp`)

Boost.Compute is a C++ wrapper for OpenCL, providing STL-style algorithms that execute on GPUs and other accelerators.

**Key features:**
- `compute::device` / `compute::context` / `compute::command_queue`
- `compute::vector<T>` — device-side container
- STL algorithms: `compute::transform`, `compute::sort`, `compute::reduce`
- Lambda-based kernels via `BOOST_COMPUTE_LAMBDA`

**Compile (requires OpenCL):**
```bash
g++ -std=c++17 boost_compute.cpp -lOpenCL -o boost_compute
```

---

### 6. Boost.Fiber — Lightweight Cooperative Concurrency (`boost_fiber.cpp`)

Boost.Fiber provides user-space, cooperatively scheduled fibers (green threads). Each fiber has its own stack but shares the OS thread, making millions of concurrent logical tasks feasible with low overhead — ideal for task-parallel HPC workloads.

**Key features:**
- `boost::fibers::fiber` — launch a fiber
- `boost::fibers::mutex` / `condition_variable` — fiber-aware synchronisation
- `boost::fibers::future` / `promise` / `packaged_task`
- `boost::fibers::buffered_channel` / `unbuffered_channel` — CSP-style communication
- Work-stealing scheduler (`boost::fibers::use_scheduling_algorithm<boost::fibers::algo::work_stealing>`)

**Compile:**
```bash
g++ -std=c++17 boost_fiber.cpp -lboost_fiber -lboost_context -pthread -o boost_fiber
```

---

## Summary Table

| Library         | Use Case                              | Parallelism Model      |
|-----------------|---------------------------------------|------------------------|
| Boost.Asio      | Async I/O, event-driven servers       | Async / Reactive       |
| Boost.Thread    | Multi-threading, RW locks, barriers   | Shared-memory threads  |
| Boost.Lockfree  | Lock-free producer-consumer pipelines | Shared-memory threads  |
| Boost.MPI       | Distributed cluster computing         | Distributed memory MPI |
| Boost.Compute   | GPU / OpenCL acceleration             | Data-parallel GPU      |
| Boost.Fiber     | Millions of lightweight tasks         | Cooperative user-space |

---

## Further Reading

- [Boost Documentation](https://www.boost.org/doc/libs/)
- [Boost.Asio Overview](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [Boost.MPI Tutorial](https://www.boost.org/doc/libs/release/doc/html/mpi/tutorial.html)
- [Boost.Compute Tutorial](https://www.boost.org/doc/libs/release/libs/compute/doc/html/boost_compute/tutorial.html)
- [Boost.Fiber Documentation](https://www.boost.org/doc/libs/release/libs/fiber/doc/html/index.html)
