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

## Examples (Advanced — Expert Level)

Each file is a self-contained program that **combines multiple Boost APIs**
to solve a realistic HPC scenario.  Read the top-of-file comment block for
the full list of facilities used and the design rationale.

---

### 1. Boost.Asio — Async Connection Pool (`boost_asio_async.cpp`)

**Scenario:** A pool of logical connections (think DB pool or HTTP keep-alive
pool) backed by a `thread_pool` executor.  Each connection owns a
`strand<thread_pool::executor_type>` that serialises all its state mutations —
eliminating the need for any explicit mutex inside the connection class.  An
`idle_timer` recycles connections that haven't received work for a configurable
period.  A `steady_timer`-based heartbeat prints live per-connection
throughput every second.

**APIs combined:**
| API | Role |
|-----|------|
| `boost::asio::thread_pool` | Multi-threaded executor |
| `asio::strand<thread_pool::executor_type>` | Per-connection serialisation |
| `asio::steady_timer` | Idle timeout + periodic heartbeat |
| `asio::post` / `asio::dispatch` | Schedule vs. run-if-on-strand |
| `boost::system::error_code` | Uniform error propagation |
| `std::enable_shared_from_this` | Safe handler lifetime capture |

**Compile:**
```bash
g++ -std=c++17 boost_asio_async.cpp -lboost_system -pthread -o boost_asio_async
```

---

### 2. Boost.Thread — Parallel Merge Sort + Kogge-Stone Scan (`boost_thread.cpp`)

**Scenario A:** Parallel merge sort using `boost::async` to sort N chunks
concurrently, `boost::when_all` to await all chunk-sort futures, then a
`future::then` continuation that merges the sorted chunks.  A
`shared_mutex`-protected `SortCache` memoises results: concurrent lookups use
`shared_lock`; inserts use `unique_lock`.

**Scenario B:** Parallel inclusive prefix-sum (Kogge-Stone algorithm) using a
`thread_group` of N workers separated by a `barrier` at each phase boundary —
the canonical Bulk-Synchronous Parallel (BSP) pattern.

**APIs combined:**
| API | Role |
|-----|------|
| `boost::async(launch::async, ...)` | Fire-and-forget parallel tasks |
| `boost::when_all(begin, end)` | Barrier over a range of futures |
| `future::then(continuation)` | Phase-2 merge chained onto phase-1 |
| `boost::shared_mutex` | Read-heavy result cache |
| `boost::thread_group` | Spawn / join N workers |
| `boost::barrier` | BSP phase gate |

**Compile:**
```bash
g++ -std=c++17 \
    -DBOOST_THREAD_PROVIDES_FUTURE \
    -DBOOST_THREAD_PROVIDES_FUTURE_CONTINUATION \
    -DBOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY \
    boost_thread.cpp \
    -lboost_thread -lboost_system -pthread -o boost_thread
```

---

### 3. Boost.Lockfree — Multi-Stage Streaming Pipeline (`boost_lockfree.cpp`)

**Scenario:** A three-stage event-processing pipeline where every handoff
between stages is lock-free and zero-allocation:

```
Generator ──[spsc_queue]──► Enricher ──[mpmc_queue]──► Aggregators (×4)
    ▲                                                          │
    └──────────────── [stack: free-list pool] ─────────────────┘
```

Events are pre-allocated in a static arena and recycled through a
`lockfree::stack` free-list.  Aggregators drain the MPMC queue with
`consume_all()`, which amortises CAS overhead across a batch.  Exponential
backoff prevents spinning a hot core under back-pressure.

**APIs combined:**
| API | Role |
|-----|------|
| `lockfree::spsc_queue` | Single-producer/single-consumer (stage 0→1) |
| `lockfree::queue` | MPMC bounded queue (stage 1→2) |
| `lockfree::stack` | Free-list object pool (zero allocation) |
| `queue::consume_all(cb)` | Batch-drain with amortised CAS |
| `std::atomic` | Per-stage throughput counters |

**Compile:**
```bash
g++ -std=c++17 boost_lockfree.cpp -pthread -o boost_lockfree
```

---

### 4. Boost.Fiber — Work-Stealing Map-Reduce (`boost_fiber.cpp`)

**Scenario:** Monte Carlo π estimation spread across `TASK_COUNT` fibers
executing on `OS_THREADS` OS threads via the work-stealing scheduler.
`FIBER_WORKERS` permanent worker fibers drain a `buffered_channel` of task
lambdas; results are accumulated under a `fibers::mutex`.  A
`packaged_task`/`future` dependency chain demonstrates fiber-suspending
upstream waits.  A `fibers::barrier` checkpoints all workers between phases.

**APIs combined:**
| API | Role |
|-----|------|
| `fibers::algo::work_stealing(N)` | Migrate ready fibers to idle OS threads |
| `fibers::buffered_channel<F>` | CSP bounded task queue |
| `fibers::packaged_task / future` | Async result with fiber-blocking `.get()` |
| `fibers::barrier` | Phase-checkpoint across fibers |
| `fibers::mutex` | Fiber-aware mutual exclusion |
| `fibers::this_fiber::yield()` | Cooperative context switch |

**Compile:**
```bash
g++ -std=c++17 boost_fiber.cpp \
    -lboost_fiber -lboost_context -pthread -o boost_fiber
```

---

### 5. Boost.MPI — Distributed 1-D Heat Equation Solver (`boost_mpi.cpp`)

**Scenario:** Explicit-Euler finite-difference solver for `u_t = α u_xx`
decomposed across MPI ranks.  Each step overlaps communication with
computation: `isend`/`irecv` posts halo exchanges in the background while the
inner cells are updated, then `mpi::wait_all` completes before the border cells
are touched.  `mpi::all_reduce` checks global convergence.  A custom
`Diagnostics` struct (with `serialize()`) is gathered on rank 0 at the end.
`communicator::split()` creates even/odd sub-communicators to demonstrate
hierarchical parallelism.

**APIs combined:**
| API | Role |
|-----|------|
| `mpi::scatterv / gatherv` | Distribute / collect unequal slices |
| `world.isend / irecv` | Non-blocking halo exchange |
| `mpi::wait_all` | Await outstanding requests |
| `mpi::all_reduce` | Global convergence check |
| `boost::serialization` | Custom struct sent as MPI message |
| `mpi::gather` | Collect per-rank Diagnostics |
| `mpi::timer` | Wall-clock timing per rank |
| `communicator::split()` | Sub-communicator groups |

**Compile and run:**
```bash
mpic++ -std=c++17 boost_mpi.cpp \
       -lboost_mpi -lboost_serialization -o boost_mpi
mpirun -n 4 ./boost_mpi
```

---

### 6. Boost.Compute — GPU Histogram Equalisation Pipeline (`boost_compute.cpp`)

**Scenario:** Full histogram equalisation pipeline executed on the GPU:

```
d_raw (uint8) → normalise → d_norm (float)
              → (CPU histogram)
              → inclusive_scan → d_cdf
              → (CPU build LUT)
              → bin_index → d_bins (int)
              → gather(LUT) → d_out (uint8)
```

`BOOST_COMPUTE_FUNCTION` defines two inline OpenCL device functions.
`compute::event` with `CL_QUEUE_PROFILING_ENABLE` measures GPU kernel
execution time.  `compute::gather` implements the LUT-apply step without
a custom kernel.

**APIs combined:**
| API | Role |
|-----|------|
| `compute::transform` + `BOOST_COMPUTE_FUNCTION` | Normalise and bin pixels |
| `compute::inclusive_scan` | Parallel CDF on the device |
| `compute::gather` | Apply equalisation LUT |
| `compute::event` profiling | GPU kernel timing |
| `command_queue::enable_profiling` | Enable CL timestamp collection |
| `compute::copy / copy_async` | Host↔device transfers |

**Compile (requires OpenCL):**
```bash
g++ -std=c++17 boost_compute.cpp -lOpenCL -o boost_compute
```

---

## Summary Table

| File | Library | HPC Pattern |
|------|---------|-------------|
| `boost_asio_async.cpp` | Boost.Asio | Async reactive server, strand-per-object |
| `boost_thread.cpp` | Boost.Thread | Parallel sort + BSP scan |
| `boost_lockfree.cpp` | Boost.Lockfree | Multi-stage zero-allocation pipeline |
| `boost_fiber.cpp` | Boost.Fiber | Work-stealing fiber map-reduce |
| `boost_mpi.cpp` | Boost.MPI | Distributed stencil with latency hiding |
| `boost_compute.cpp` | Boost.Compute | GPU image-processing pipeline |

---

## Further Reading

- [Boost Documentation](https://www.boost.org/doc/libs/)
- [Boost.Asio Executor Model](https://www.boost.org/doc/libs/release/doc/html/boost_asio/overview/core/basics.html)
- [Boost.Thread futures and continuations](https://www.boost.org/doc/libs/release/doc/html/thread/synchronization.html)
- [Boost.Lockfree design notes](https://www.boost.org/doc/libs/release/doc/html/lockfree.html)
- [Boost.Fiber work-stealing](https://www.boost.org/doc/libs/release/libs/fiber/doc/html/fiber/scheduling.html)
- [Boost.MPI Tutorial](https://www.boost.org/doc/libs/release/doc/html/mpi/tutorial.html)
- [Boost.Compute Tutorial](https://www.boost.org/doc/libs/release/libs/compute/doc/html/boost_compute/tutorial.html)
