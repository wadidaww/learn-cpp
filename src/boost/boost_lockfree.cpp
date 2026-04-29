// boost_lockfree.cpp
// Demonstrates Boost.Lockfree data structures for HPC producer-consumer pipelines.
//
// Compile (header-only, no extra link flags):
//   g++ -std=c++17 boost_lockfree.cpp -pthread -o boost_lockfree
//
// Concepts shown:
//   - spsc_queue : single-producer / single-consumer ring buffer (fastest)
//   - queue      : multi-producer / multi-consumer bounded queue
//   - stack      : multi-producer / multi-consumer lock-free stack

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/stack.hpp>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Example 1: spsc_queue — single producer / single consumer
//
// The ring buffer is the fastest lock-free structure when only one thread
// writes and one thread reads. Typical use: audio pipelines, sensor streams.
// ---------------------------------------------------------------------------
void spsc_queue_example() {
    // Compile-time capacity of 1024 elements.
    boost::lockfree::spsc_queue<int, boost::lockfree::capacity<1024>> q;

    std::atomic<bool> done{false};
    long long sum = 0;

    std::thread producer([&] {
        for (int i = 0; i < 10'000; ++i) {
            while (!q.push(i)) { /* spin until space available */ }
        }
        done = true;
    });

    std::thread consumer([&] {
        int val;
        while (!done || q.read_available()) {
            if (q.pop(val)) sum += val;
        }
    });

    producer.join();
    consumer.join();
    std::cout << "[spsc_queue] sum 0..9999 = " << sum
              << "  (expected " << (9999LL * 10000 / 2) << ")\n";
}

// ---------------------------------------------------------------------------
// Example 2: queue — multiple producers and multiple consumers
//
// boost::lockfree::queue is safe for any number of concurrent push/pop calls.
// Items are processed in roughly FIFO order (no hard ordering guarantee).
// ---------------------------------------------------------------------------
void mpmc_queue_example() {
    // Runtime capacity: 128 slots (must be a power of two).
    boost::lockfree::queue<int> q(128);

    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};

    // 4 producers each push 250 items.
    std::vector<std::thread> producers;
    for (int p = 0; p < 4; ++p) {
        producers.emplace_back([&, p] {
            for (int i = 0; i < 250; ++i) {
                int val = p * 1000 + i;
                while (!q.push(val)) { /* back-pressure: queue full */ }
                ++produced;
            }
        });
    }

    // 4 consumers drain the queue.
    std::atomic<bool> stop{false};
    std::vector<std::thread> consumers;
    for (int c = 0; c < 4; ++c) {
        consumers.emplace_back([&] {
            int val;
            while (!stop || q.unsynchronized_empty() == false) {
                if (q.pop(val)) ++consumed;
            }
        });
    }

    for (auto& t : producers) t.join();
    stop = true;
    for (auto& t : consumers) t.join();

    std::cout << "[mpmc_queue] produced=" << produced
              << "  consumed=" << consumed << "\n";
}

// ---------------------------------------------------------------------------
// Example 3: stack — lock-free LIFO
//
// Useful for free-list memory pools and work-stealing schedulers.
// ---------------------------------------------------------------------------
void lockfree_stack_example() {
    boost::lockfree::stack<int> stk(64);

    // Push items.
    for (int i = 1; i <= 10; ++i) stk.push(i);

    // Pop and print (LIFO order).
    std::cout << "[lockfree_stack] popped:";
    int val;
    while (stk.pop(val)) std::cout << " " << val;
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== spsc_queue (single producer / single consumer) ===\n";
    spsc_queue_example();

    std::cout << "\n=== MPMC queue (multiple producers / multiple consumers) ===\n";
    mpmc_queue_example();

    std::cout << "\n=== lock-free stack ===\n";
    lockfree_stack_example();

    return 0;
}
