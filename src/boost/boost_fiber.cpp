// boost_fiber.cpp
// Demonstrates Boost.Fiber for lightweight cooperative concurrency in HPC.
//
// Fibers are user-space cooperative threads. Millions can exist simultaneously
// with far lower overhead than OS threads. Each fiber voluntarily yields
// control, making them ideal for task-parallel workloads and async I/O overlap.
//
// Compile:
//   g++ -std=c++17 boost_fiber.cpp -lboost_fiber -lboost_context -pthread -o boost_fiber
//
// Concepts shown:
//   - Launch fibers and join them
//   - boost::fibers::mutex / condition_variable
//   - boost::fibers::future / promise / packaged_task
//   - buffered_channel — CSP-style producer-consumer
//   - Work-stealing scheduler for multi-threaded fiber execution

#include <boost/fiber/all.hpp>
#include <iostream>
#include <vector>
#include <numeric>

namespace fibers = boost::fibers;

// ---------------------------------------------------------------------------
// Example 1: Basic fiber launch and join
// ---------------------------------------------------------------------------
void basic_fiber_example() {
    std::vector<fibers::fiber> fvec;

    for (int i = 0; i < 6; ++i) {
        fvec.emplace_back([i] {
            std::cout << "[fiber] fiber " << i << " running\n";
            fibers::this_fiber::yield(); // Voluntarily surrender the scheduler.
            std::cout << "[fiber] fiber " << i << " resumed\n";
        });
    }

    for (auto& f : fvec) f.join();
}

// ---------------------------------------------------------------------------
// Example 2: Fiber mutex and condition_variable
//
// The fiber-aware mutex/condvar suspend the *fiber* (not the OS thread) while
// waiting, allowing other fibers on the same thread to make progress.
// ---------------------------------------------------------------------------
void mutex_condvar_example() {
    fibers::mutex              mtx;
    fibers::condition_variable cv;
    bool                       ready = false;
    int                        shared_data = 0;

    fibers::fiber producer([&] {
        {
            std::unique_lock<fibers::mutex> lk(mtx);
            shared_data = 99;
            ready = true;
        }
        cv.notify_one();
        std::cout << "[condvar] producer set data=" << shared_data << "\n";
    });

    fibers::fiber consumer([&] {
        std::unique_lock<fibers::mutex> lk(mtx);
        cv.wait(lk, [&] { return ready; });
        std::cout << "[condvar] consumer read data=" << shared_data << "\n";
    });

    producer.join();
    consumer.join();
}

// ---------------------------------------------------------------------------
// Example 3: fiber promise / future
//
// Identical API to std::promise/future but suspends the fiber rather than
// the OS thread, so other fibers can run while waiting for the result.
// ---------------------------------------------------------------------------
void promise_future_example() {
    fibers::promise<double> prom;
    fibers::future<double>  fut = prom.get_future();

    fibers::fiber worker([&prom] {
        double sum = 0.0;
        for (int i = 1; i <= 100'000; ++i) sum += 1.0 / i;
        prom.set_value(sum);
    });

    // This fiber suspends here until the worker sets the value.
    double result = fut.get();
    std::cout << "[future] harmonic sum = " << result << "\n";

    worker.join();
}

// ---------------------------------------------------------------------------
// Example 4: buffered_channel — CSP producer-consumer pipeline
//
// A buffered_channel<T> is a thread-safe (between fibers on the same thread)
// FIFO channel. The producer pushes values; the consumer pops them.
// The channel signals end-of-stream via channel_op_status::closed.
// ---------------------------------------------------------------------------
void channel_example() {
    fibers::buffered_channel<int> ch(/*capacity=*/8);

    fibers::fiber producer([&ch] {
        for (int i = 0; i < 10; ++i) {
            ch.push(i); // Suspends if the buffer is full.
        }
        ch.close(); // Signal EOF to the consumer.
        std::cout << "[channel] producer done\n";
    });

    fibers::fiber consumer([&ch] {
        int val;
        long long sum = 0;
        while (fibers::channel_op_status::success == ch.pop(val)) {
            sum += val;
        }
        std::cout << "[channel] consumer received sum=" << sum << "\n";
    });

    producer.join();
    consumer.join();
}

// ---------------------------------------------------------------------------
// Example 5: packaged_task — wrap any callable as an async fiber task
// ---------------------------------------------------------------------------
void packaged_task_example() {
    fibers::packaged_task<int(int, int)> task([](int a, int b) {
        fibers::this_fiber::yield(); // Simulate work.
        return a + b;
    });

    fibers::future<int> fut = task.get_future();
    fibers::fiber        f(std::move(task), 21, 21);

    std::cout << "[packaged_task] 21 + 21 = " << fut.get() << "\n";
    f.join();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== Basic fiber launch / yield / join ===\n";
    basic_fiber_example();

    std::cout << "\n=== Fiber mutex + condition_variable ===\n";
    mutex_condvar_example();

    std::cout << "\n=== Fiber promise / future ===\n";
    promise_future_example();

    std::cout << "\n=== Buffered channel (CSP pipeline) ===\n";
    channel_example();

    std::cout << "\n=== Packaged task ===\n";
    packaged_task_example();

    return 0;
}
