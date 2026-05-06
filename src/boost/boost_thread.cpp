// boost_thread.cpp — Advanced
//
// Two combined demonstrations of Boost.Thread for HPC:
//
//  A) Parallel merge-sort driven by boost::async + when_all + future::then
//     • boost::async(launch::async, ...)  — fire-and-forget worker threads
//     • boost::when_all(iterators)        — barrier over a range of futures
//     • future::then(continuation)        — chain phase 2 onto phase 1
//     • SortCache with shared_mutex       — concurrent reads, exclusive writes
//     • boost::thread::get_id()           — identify workers in the log
//
//  B) Parallel inclusive prefix-sum (Kogge-Stone) with BSP phase sync
//     • boost::thread_group               — spawn/join N workers at once
//     • boost::barrier                    — rendezvous between every phase
//     • per-thread slice ownership        — data partitioning pattern
//
// Compile:
//   g++ -std=c++17                              \
//       -DBOOST_THREAD_PROVIDES_FUTURE          \
//       -DBOOST_THREAD_PROVIDES_FUTURE_CONTINUATION \
//       -DBOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY \
//       boost_thread.cpp                        \
//       -lboost_thread -lboost_system -pthread  \
//       -o boost_thread

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY

#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <unordered_map>
#include <vector>

// ============================================================================
//  SortCache
//  Memoises sort results keyed by the input checksum.
//  Read-heavy: many cache lookups, rare inserts.
//  shared_mutex allows all readers to proceed concurrently; writers are
//  exclusive.  This is the canonical Boost read-write lock pattern.
// ============================================================================
class SortCache {
public:
    using Key   = std::size_t;
    using Value = std::vector<int>;

    bool insert(Key k, Value v) {
        boost::unique_lock<boost::shared_mutex> wlk(mtx_);
        return cache_.emplace(k, std::move(v)).second;
    }

    std::optional<Value> find(Key k) const {
        boost::shared_lock<boost::shared_mutex> rlk(mtx_);
        auto it = cache_.find(k);
        return (it != cache_.end()) ? std::make_optional(it->second)
                                    : std::nullopt;
    }

    std::size_t size() const {
        boost::shared_lock<boost::shared_mutex> rlk(mtx_);
        return cache_.size();
    }

private:
    mutable boost::shared_mutex       mtx_;
    std::unordered_map<Key, Value>    cache_;
};

// Polynomial hash used as the cache key.
static std::size_t checksum(const std::vector<int>& v) {
    std::size_t h = 0;
    for (int x : v) h = h * 31 + static_cast<std::size_t>(x);
    return h;
}

// ============================================================================
//  parallel_sort
//
//  Returns boost::future<vector<int>> for the sorted result.
//
//  Phase 1  (parallel, async):  boost::async sorts N independent chunks.
//  Phase 2  (serial, via .then): bottom-up merges in a single continuation.
//
//  boost::when_all drives the phase-1 → phase-2 transition: the continuation
//  runs only after every chunk-sort future is ready, with no blocking wait
//  on the calling thread.
// ============================================================================
boost::future<std::vector<int>>
parallel_sort(std::vector<int> input, int nthreads) {
    // Heap-allocate the data so it survives async lambdas.
    auto data  = std::make_shared<std::vector<int>>(std::move(input));
    int  N     = static_cast<int>(data->size());
    int  chunk = (N + nthreads - 1) / nthreads;

    // ---- Phase 1: parallel chunk-sort ----------------------------------------
    std::vector<boost::future<void>> phase1;
    phase1.reserve(nthreads);
    for (int t = 0; t < nthreads; ++t) {
        int lo = t * chunk;
        int hi = std::min(lo + chunk, N);
        phase1.push_back(boost::async(boost::launch::async,
            [data, lo, hi] {
                std::sort(data->begin() + lo, data->begin() + hi);
                std::cout << "  [thread " << boost::this_thread::get_id()
                          << "] sorted slice [" << lo << ", " << hi << ")\n";
            }));
    }

    // ---- Phase 2: sequential bottom-up merge in a continuation ---------------
    // when_all(begin, end) returns future<vector<future<void>>> that becomes
    // ready once every element-future is ready.  .then() chains a callback.
    return boost::when_all(phase1.begin(), phase1.end())
        .then([data, chunk, N]
              (boost::future<std::vector<boost::future<void>>>) mutable {
            auto& d = *data;
            // Bottom-up merge: double the merge width each pass.
            for (int step = chunk; step < N; step *= 2) {
                for (int i = 0; i < N; i += 2 * step) {
                    auto mid = d.begin() + std::min(i + step,     N);
                    auto end = d.begin() + std::min(i + 2 * step, N);
                    std::inplace_merge(d.begin() + i, mid, end);
                }
            }
            return std::move(*data);
        });
}

// ============================================================================
//  parallel_prefix_sum  (inclusive scan)
//
//  Kogge-Stone algorithm:
//    Phase k:  x[i] += x[i - 2^k]   for all i >= 2^k, in parallel
//  Requires ceil(log2 N) barrier-separated phases.
//
//  boost::thread_group spawns N workers that share a single barrier.
//  The barrier enforces that all threads finish writing buf before any
//  thread reads back the new values — the textbook BSP superstep pattern.
// ============================================================================
void parallel_prefix_sum(std::vector<long long>& data, int nthreads) {
    const int N      = static_cast<int>(data.size());
    const int chunk  = (N + nthreads - 1) / nthreads;
    const int phases = static_cast<int>(std::ceil(std::log2(N)));

    // Double-buffer: workers write into buf, then we swap into data.
    std::vector<long long> buf(N);

    // Barrier shared across all workers.  After each phase all threads
    // synchronise twice: once after writing buf, once after copying buf→data.
    auto bar = std::make_shared<boost::barrier>(nthreads);

    boost::thread_group workers;
    for (int tid = 0; tid < nthreads; ++tid) {
        workers.create_thread([&data, &buf, N, chunk, phases, tid, bar] {
            int lo = tid * chunk;
            int hi = std::min(lo + chunk, N);

            for (int ph = 0; ph < phases; ++ph) {
                int stride = 1 << ph;   // 2^ph

                // Write phase: each thread updates its slice into buf.
                for (int i = lo; i < hi; ++i)
                    buf[i] = data[i] + (i >= stride ? data[i - stride] : 0LL);

                bar->wait();  // all writes to buf complete

                // Copy phase: promote buf → data for the next phase.
                for (int i = lo; i < hi; ++i)
                    data[i] = buf[i];

                bar->wait();  // all copies complete before next write phase
            }
        });
    }
    workers.join_all();
}

// ============================================================================
//  main
// ============================================================================
int main() {
    // ---- A: Parallel merge-sort ------------------------------------------------
    std::cout << "=== Parallel merge-sort"
                 " (boost::async + when_all + future::then) ===\n";

    const int SORT_N = 32;
    std::vector<int> input(SORT_N);
    std::mt19937 rng(42);
    std::iota(input.begin(), input.end(), 1);
    std::shuffle(input.begin(), input.end(), rng);

    std::cout << "Input  (first 16):";
    for (int i = 0; i < 16; ++i) std::cout << " " << std::setw(3) << input[i];
    std::cout << "\n";

    SortCache cache;
    std::size_t key = checksum(input);

    if (auto cached = cache.find(key)) {
        std::cout << "Cache hit — skipping sort\n";
    } else {
        // parallel_sort returns immediately; .get() blocks until the
        // when_all continuation has finished.
        auto sorted = parallel_sort(input, 4).get();

        std::cout << "Sorted (first 16):";
        for (int i = 0; i < 16; ++i)
            std::cout << " " << std::setw(3) << sorted[i];
        std::cout << "\n";

        cache.insert(key, sorted);   // exclusive (unique_lock) write
        std::cout << "Inserted into cache  size=" << cache.size() << "\n";
    }

    // Re-query to exercise the shared_lock read path.
    if (auto result = cache.find(key))
        std::cout << "Cache lookup: sorted[0]=" << (*result)[0]
                  << "  sorted[31]=" << (*result)[31] << "\n";

    // ---- B: Parallel prefix-sum ------------------------------------------------
    std::cout << "\n=== Parallel inclusive prefix-sum"
                 " (thread_group + barrier, Kogge-Stone) ===\n";

    const int PS_N = 16;
    std::vector<long long> pdata(PS_N);
    std::iota(pdata.begin(), pdata.end(), 1LL);   // 1, 2, …, 16

    std::cout << "Input:";
    for (auto v : pdata) std::cout << " " << v;
    std::cout << "\n";

    parallel_prefix_sum(pdata, 4);

    std::cout << "Scan: ";
    for (auto v : pdata) std::cout << " " << v;
    std::cout << "\n";
    // Expected: 1 3 6 10 15 21 28 36 45 55 66 78 91 105 120 136

    return 0;
}
