// boost_lockfree.cpp — Advanced
//
// Multi-stage lock-free streaming pipeline with a zero-allocation free-list.
//
// Pipeline topology:
//
//   Generator ──[spsc_queue]──► Enricher ──[mpmc_queue]──► Aggregators(N)
//       ▲                                                         │
//       └───────────────[stack: free-list pool]──────────────────┘
//
// Facilities combined:
//   boost::lockfree::spsc_queue   — stage 0→1 (one producer, one consumer)
//                                   wait-free, cache-line friendly ring buffer
//   boost::lockfree::queue        — stage 1→2 (one producer, N consumers)
//                                   CAS-based MPMC bounded queue
//   boost::lockfree::stack        — object-pool free-list: Events are
//                                   pre-allocated; returned to the pool after
//                                   processing — zero heap allocation in steady
//                                   state, cache-hot reuse
//   queue::consume_all(callback)  — batch-drain a queue with one callback per
//                                   item; amortises CAS overhead vs pop()-loop
//   std::atomic                   — per-stage throughput counters (no mutex)
//   Exponential backoff           — progressive sleep to avoid spinning a hot
//                                   CPU core under back-pressure
//
// Compile (lockfree is header-only):
//   g++ -std=c++17 boost_lockfree.cpp -pthread -o boost_lockfree

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/stack.hpp>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ============================================================================
//  Event — the item flowing through the pipeline.
//  Allocated once at startup from a static arena; recycled via the free-list.
// ============================================================================
struct Event {
    int    id;
    double value;
    int    tag;       // classification tag added by the enricher
    bool   processed; // marked by an aggregator
};

// ============================================================================
//  Pipeline constants
// ============================================================================
static constexpr int POOL_SIZE       = 512;
static constexpr int SPSC_CAPACITY   = 256;
static constexpr int MPMC_CAPACITY   = 256;
static constexpr int TOTAL_EVENTS    = 50'000;
static constexpr int NUM_AGGREGATORS = 4;

// ============================================================================
//  Shared lock-free structures
// ============================================================================
// Free-list: compile-time capacity, no dynamic allocation.
boost::lockfree::stack<Event*, boost::lockfree::capacity<POOL_SIZE>> free_list;

// Stage 0→1: generator writes, enricher reads (spsc = no CAS on fast path).
boost::lockfree::spsc_queue<Event*,
    boost::lockfree::capacity<SPSC_CAPACITY>>                         stage01;

// Stage 1→2: enricher writes, aggregators race to consume (MPMC).
boost::lockfree::queue<Event*>                                        stage12{MPMC_CAPACITY};

// Per-stage counters; relaxed loads/stores are sufficient for stats.
std::atomic<long> cnt_generated{0};
std::atomic<long> cnt_enriched{0};
std::atomic<long> cnt_aggregated{0};

// Termination signals.
std::atomic<bool> generator_done{false};
std::atomic<bool> enricher_done{false};

// ============================================================================
//  Backoff
//  Progressive sleep strategy: busy-spin → yield → sleep.
//  Applied whenever a push/pop fails (back-pressure or empty queue).
// ============================================================================
struct Backoff {
    void spin() {
        if      (n_ < 4)  { /* tight spin */ }
        else if (n_ < 16) { std::this_thread::yield(); }
        else              { std::this_thread::sleep_for(50us); }
        ++n_;
    }
    void reset() noexcept { n_ = 0; }
private:
    int n_{0};
};

// ============================================================================
//  Stage 0: Generator
//  Pops an Event from the free-list, fills it in, and pushes it into stage01.
//  Back-pressure: if the free-list or spsc_queue is full, the generator backs
//  off — the downstream stages are the pacemaker.
// ============================================================================
void generator() {
    Backoff bo;
    for (int i = 0; i < TOTAL_EVENTS; ++i) {
        Event* ev = nullptr;
        while (!free_list.pop(ev)) bo.spin();   // wait for a recycled slot
        bo.reset();

        ev->id        = i;
        ev->value     = static_cast<double>(i) * 0.001;
        ev->tag       = 0;
        ev->processed = false;

        while (!stage01.push(ev)) bo.spin();    // back-pressure from enricher
        bo.reset();
        ++cnt_generated;
    }
    generator_done.store(true, std::memory_order_release);
    std::cout << "[generator ] done  n=" << cnt_generated.load() << "\n";
}

// ============================================================================
//  Stage 1: Enricher
//  Reads from stage01 (spsc), classifies the event, pushes into stage12 (mpmc).
// ============================================================================
void enricher() {
    Event*  ev = nullptr;
    Backoff bo;
    while (true) {
        if (stage01.pop(ev)) {
            bo.reset();
            ev->tag = ev->id % 8;   // classify into 8 buckets
            Backoff push_bo;
            while (!stage12.push(ev)) push_bo.spin();
            ++cnt_enriched;
        } else {
            // Drain any residual items after the generator signals done.
            if (generator_done.load(std::memory_order_acquire)
                    && stage01.empty())
                break;
            bo.spin();
        }
    }
    enricher_done.store(true, std::memory_order_release);
    std::cout << "[enricher  ] done  n=" << cnt_enriched.load() << "\n";
}

// ============================================================================
//  Stage 2: Aggregator (one of NUM_AGGREGATORS)
//
//  Uses consume_all() which drains all currently-visible items in a single
//  sweep, invoking the callback once per item.  This batches the CAS
//  operations on the MPMC queue's head pointer, which is more efficient than
//  calling pop() in a tight loop.
// ============================================================================
void aggregator(int id) {
    long local_n = 0;
    Backoff bo;
    while (true) {
        std::size_t n = stage12.consume_all([&](Event* ev) {
            ev->processed = true;
            free_list.push(ev);   // return to pool — enables the generator
            ++local_n;
            ++cnt_aggregated;
        });
        if (n == 0) {
            if (enricher_done.load(std::memory_order_acquire)
                    && stage12.empty())
                break;
            bo.spin();
        } else {
            bo.reset();
        }
    }
    std::cout << "[aggregator " << id << "] done  local_n=" << local_n << "\n";
}

// ============================================================================
//  main
// ============================================================================
int main() {
    std::cout << "=== Multi-stage lock-free pipeline"
                 " (spsc → mpmc + free-list pool) ===\n\n";

    // Pre-populate the free-list with Events from a static arena.
    // This is the only allocation; the pipeline itself is allocation-free.
    static Event arena[POOL_SIZE];
    for (int i = 0; i < POOL_SIZE; ++i)
        free_list.push(&arena[i]);

    auto t0 = std::chrono::steady_clock::now();

    std::thread t_gen(generator);
    std::thread t_enr(enricher);
    std::vector<std::thread> t_agg;
    for (int i = 0; i < NUM_AGGREGATORS; ++i)
        t_agg.emplace_back(aggregator, i);

    t_gen.join();
    t_enr.join();
    for (auto& t : t_agg) t.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();

    std::cout << "\n--- Pipeline stats ---\n"
              << "  generated  = " << cnt_generated.load()  << "\n"
              << "  enriched   = " << cnt_enriched.load()   << "\n"
              << "  aggregated = " << cnt_aggregated.load() << "\n"
              << "  wall time  = " << ms << " ms\n"
              << "  throughput ≈ "
              << (cnt_aggregated.load() * 1000L / std::max(1L, (long)ms))
              << " events/sec\n";

    return 0;
}
