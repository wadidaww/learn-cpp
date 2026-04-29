// boost_thread.cpp
// Demonstrates Boost.Thread features useful for HPC.
//
// Compile:
//   g++ -std=c++17 boost_thread.cpp -lboost_thread -lboost_system -pthread -o boost_thread
//
// Concepts shown:
//   - boost::thread_group  : manage a set of threads as a unit
//   - boost::shared_mutex  : reader-writer lock (many readers / one writer)
//   - boost::barrier       : synchronisation point for a fixed number of threads
//   - boost::future/promise: asynchronous result passing

#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/future.hpp>
#include <iostream>
#include <vector>
#include <numeric>

// ---------------------------------------------------------------------------
// Example 1: thread_group — launch and join many threads at once
// ---------------------------------------------------------------------------
void thread_group_example() {
    boost::thread_group group;

    for (int i = 0; i < 6; ++i) {
        group.create_thread([i] {
            std::cout << "[thread_group] thread " << i
                      << " id=" << boost::this_thread::get_id() << "\n";
        });
    }

    group.join_all(); // Block until every thread in the group has finished.
}

// ---------------------------------------------------------------------------
// Example 2: shared_mutex — concurrent reads, exclusive writes
//
// In HPC workloads a shared data structure is often read far more than it is
// written. shared_mutex allows all reader threads to proceed in parallel while
// a writer gets exclusive access.
// ---------------------------------------------------------------------------
static int shared_value = 0;
static boost::shared_mutex rw_mutex;

void reader(int id) {
    boost::shared_lock<boost::shared_mutex> lock(rw_mutex); // shared (read) lock
    std::cout << "[shared_mutex] reader " << id
              << " sees value=" << shared_value << "\n";
}

void writer(int new_val) {
    boost::unique_lock<boost::shared_mutex> lock(rw_mutex); // exclusive (write) lock
    shared_value = new_val;
    std::cout << "[shared_mutex] writer set value=" << shared_value << "\n";
}

void shared_mutex_example() {
    boost::thread_group group;
    for (int i = 0; i < 4; ++i) group.create_thread([i] { reader(i); });
    group.create_thread([] { writer(42); });
    for (int i = 4; i < 8; ++i) group.create_thread([i] { reader(i); });
    group.join_all();
}

// ---------------------------------------------------------------------------
// Example 3: barrier — rendezvous point for parallel phases
//
// Classic HPC pattern: all workers complete phase N before any starts phase N+1.
// ---------------------------------------------------------------------------
void barrier_example() {
    const int NUM_THREADS = 4;
    boost::barrier bar(NUM_THREADS);

    boost::thread_group group;
    for (int i = 0; i < NUM_THREADS; ++i) {
        group.create_thread([i, &bar] {
            std::cout << "[barrier] thread " << i << " finishing phase 1\n";
            bar.wait(); // All threads must arrive here before any continues.
            std::cout << "[barrier] thread " << i << " starting  phase 2\n";
        });
    }
    group.join_all();
}

// ---------------------------------------------------------------------------
// Example 4: promise / future — pass results between threads
// ---------------------------------------------------------------------------
void promise_future_example() {
    boost::promise<double> prom;
    boost::future<double>  fut = prom.get_future();

    // Worker thread: heavy computation, result delivered via promise.
    boost::thread worker([&prom] {
        double result = 0.0;
        for (int i = 1; i <= 1'000'000; ++i) result += 1.0 / i;
        prom.set_value(result);
    });

    // Main thread: do other work, then block on the result.
    std::cout << "[promise/future] harmonic sum = " << fut.get() << "\n";
    worker.join();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== thread_group ===\n";
    thread_group_example();

    std::cout << "\n=== shared_mutex (reader-writer lock) ===\n";
    shared_mutex_example();

    std::cout << "\n=== barrier (phase synchronisation) ===\n";
    barrier_example();

    std::cout << "\n=== promise / future ===\n";
    promise_future_example();

    return 0;
}
