// boost_fiber.cpp — Advanced
//
// Fiber-based parallel map-reduce with work-stealing across OS threads.
//
// Facilities combined:
//   boost::fibers::algo::work_stealing  — automatically migrates ready fibers
//     onto idle OS threads; the key primitive for multi-core fiber execution
//   boost::fibers::buffered_channel     — CSP-style bounded task queue; worker
//     fibers block (not spin) when the channel is empty, freeing the OS thread
//     for stolen work
//   boost::fibers::packaged_task/future — per-task async result handles with
//     fiber-aware blocking (fut.get() suspends the *fiber*, not the OS thread)
//   boost::fibers::barrier              — fiber-level checkpoint; all workers
//     must arrive before any proceeds — identical semantics to std::barrier
//     but suspends fibers, not threads
//   boost::fibers::mutex/condition_variable — fiber-aware mutual exclusion
//
// Scenario: Monte Carlo estimation of π.
//   TASK_COUNT independent fibers each run a Monte Carlo quarter-circle sample.
//   FIBER_WORKERS worker fibers drain a buffered_channel of tasks.
//   OS_THREADS OS threads share the work-stealing scheduler so all cores
//   are busy even though we use far fewer OS threads than tasks.
//
// Compile:
//   g++ -std=c++17 boost_fiber.cpp \
//       -lboost_fiber -lboost_context -pthread -o boost_fiber

#include <boost/fiber/all.hpp>
#include <boost/fiber/algo/work_stealing.hpp>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

namespace fibers = boost::fibers;
namespace algo   = boost::fibers::algo;
using namespace std::chrono_literals;

static constexpr std::size_t OS_THREADS    = 4;
static constexpr std::size_t FIBER_WORKERS = 8;
static constexpr std::size_t TASK_COUNT    = 32;

// ============================================================================
//  monte_carlo_quarter_pi
//  Estimates π/4 using a simple LCG so all fibers are deterministic with
//  no thread-local state.  Yields periodically — cooperative multitasking
//  lets the work-stealing scheduler migrate the fiber to another OS thread.
// ============================================================================
static double monte_carlo_quarter_pi(std::size_t trials, std::size_t seed) {
    std::size_t inside = 0;
    const std::size_t YIELD_EVERY = trials / 8;
    std::uint64_t s = static_cast<std::uint64_t>(seed) * 6364136223846793005ULL
                      + 1442695040888963407ULL;
    for (std::size_t i = 0; i < trials; ++i) {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        double x = static_cast<double>(s >> 33) / static_cast<double>(1ULL << 31);
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        double y = static_cast<double>(s >> 33) / static_cast<double>(1ULL << 31);
        if (x * x + y * y <= 1.0) ++inside;
        if (i % YIELD_EVERY == 0)
            fibers::this_fiber::yield();   // cooperative context switch
    }
    return static_cast<double>(inside) / static_cast<double>(trials);
}

// ============================================================================
//  os_worker
//  Body of each background OS thread:
//    1. Install the work-stealing scheduler (same pool size as main thread).
//    2. Suspend at the lifecycle barrier — other fibers are stolen and run
//       here while this fiber is blocked.
// ============================================================================
static void os_worker(std::size_t nthreads, fibers::barrier* lifecycle) {
    fibers::use_scheduling_algorithm<algo::work_stealing>(nthreads);
    lifecycle->wait();   // suspends this fiber; stolen fibers execute here
}

// ============================================================================
//  main
// ============================================================================
int main() {
    std::cout << "=== Fiber work-stealing map-reduce (Monte Carlo π) ===\n\n";

    // Lifecycle barrier: main fiber + (OS_THREADS - 1) background fibers.
    fibers::barrier lifecycle(OS_THREADS);

    // Install work-stealing on the main OS thread first, then start workers.
    fibers::use_scheduling_algorithm<algo::work_stealing>(OS_THREADS);

    std::vector<std::thread> os_pool;
    os_pool.reserve(OS_THREADS - 1);
    for (std::size_t i = 1; i < OS_THREADS; ++i)
        os_pool.emplace_back(os_worker, OS_THREADS, &lifecycle);

    // ==== Phase 1: map-reduce via worker-fiber pool ========================

    // Task channel: bounded so that the submitter yields when it runs ahead.
    fibers::buffered_channel<std::function<double()>> task_ch(FIBER_WORKERS * 2);

    // Shared result accumulator; guarded by a fiber mutex.
    std::vector<double> results;
    fibers::mutex        results_mtx;

    // Launch FIBER_WORKERS fibers that drain the channel.
    std::vector<fibers::fiber> worker_fibers;
    worker_fibers.reserve(FIBER_WORKERS);
    for (std::size_t w = 0; w < FIBER_WORKERS; ++w) {
        worker_fibers.emplace_back([&task_ch, &results, &results_mtx] {
            std::function<double()> task;
            while (fibers::channel_op_status::success == task_ch.pop(task)) {
                double val = task();  // suspends internally via yield()
                std::unique_lock<fibers::mutex> lk(results_mtx);
                results.push_back(val);
            }
        });
    }

    // Submit TASK_COUNT tasks into the channel.
    // push() suspends this fiber (not the OS thread) if the channel is full.
    const std::size_t TRIALS = 200'000;
    for (std::size_t t = 0; t < TASK_COUNT; ++t) {
        task_ch.push([t, TRIALS] {
            return monte_carlo_quarter_pi(TRIALS, t + 1);
        });
    }
    task_ch.close();   // no more tasks; workers will exit their pop() loop

    for (auto& f : worker_fibers) f.join();

    double sum = 0.0;
    for (double v : results) sum += v;
    double pi = (sum / static_cast<double>(results.size())) * 4.0;

    std::cout << "Tasks completed : " << results.size() << "/" << TASK_COUNT
              << "\nπ estimate      : " << pi
              << "  (true: 3.14159…)\n\n";

    // ==== Phase 2: packaged_task chain =====================================
    // Shows future<T>.get() suspending a fiber (not the OS thread) while
    // another fiber produces the result.

    std::cout << "--- packaged_task dependency chain ---\n";

    // Task A: generate a vector.
    fibers::packaged_task<std::vector<double>()> pt_gen([] {
        std::vector<double> v(8);
        std::iota(v.begin(), v.end(), 1.0);
        fibers::this_fiber::yield();
        return v;
    });
    auto fut_gen = pt_gen.get_future();
    fibers::fiber fgen(std::move(pt_gen));

    // Task B: sum the vector produced by A.
    // fut_gen.get() suspends fiber B until fiber A fulfils its promise —
    // the work-stealing scheduler runs other fibers in the meantime.
    fibers::packaged_task<double(std::vector<double>)> pt_sum(
        [](std::vector<double> v) {
            fibers::this_fiber::yield();
            return std::accumulate(v.begin(), v.end(), 0.0);
        });
    auto fut_sum = pt_sum.get_future();
    fibers::fiber fsum(
        [pt = std::move(pt_sum), &fut_gen]() mutable {
            pt(fut_gen.get());   // fiber-suspending wait on upstream result
        });

    fgen.join();
    fsum.join();
    std::cout << "sum([1..8]) = " << fut_sum.get() << "  (expected 36)\n\n";

    // ==== Phase 3: fiber barrier checkpoint ================================
    std::cout << "--- fiber barrier (phase checkpoint) ---\n";

    constexpr int NWORKERS = 5;
    fibers::barrier chk(NWORKERS + 1);   // workers + main fiber
    std::atomic<int> phase1_done{0}, phase2_done{0};

    std::vector<fibers::fiber> chk_fibers;
    chk_fibers.reserve(NWORKERS);
    for (int i = 0; i < NWORKERS; ++i) {
        chk_fibers.emplace_back([i, &chk, &phase1_done, &phase2_done] {
            ++phase1_done;
            std::cout << "  worker " << i << " reached checkpoint\n";
            chk.wait();   // suspend fiber; proceed only when all N+1 arrive
            ++phase2_done;
            std::cout << "  worker " << i << " past checkpoint\n";
        });
    }
    chk.wait();   // main fiber arrives; all fibers now unblock together
    std::cout << "All " << phase1_done.load() << " workers passed phase 1\n";
    for (auto& f : chk_fibers) f.join();
    std::cout << "All " << phase2_done.load() << " workers passed phase 2\n\n";

    // Signal background OS threads to exit.
    lifecycle.wait();
    for (auto& t : os_pool) t.join();

    return 0;
}
