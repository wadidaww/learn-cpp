// boost_asio_async.cpp
// Demonstrates Boost.Asio for High Performance async I/O.
//
// Compile:
//   g++ -std=c++17 boost_asio_async.cpp -lboost_system -pthread -o boost_asio_async
//
// Concepts shown:
//   - io_context as an event loop
//   - async steady_timer (non-blocking wait)
//   - Running io_context on a thread pool (parallel task execution)
//   - boost::asio::strand to serialise handlers without mutexes

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

namespace asio = boost::asio;

// ---------------------------------------------------------------------------
// Example 1: Single async timer
// ---------------------------------------------------------------------------
void single_timer_example() {
    asio::io_context io;

    asio::steady_timer timer(io, std::chrono::seconds(1));

    // Post a handler to be called when the timer expires.
    timer.async_wait([](const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "[Timer] fired after 1 second\n";
        }
    });

    // Run the event loop until all pending handlers are done.
    io.run();
}

// ---------------------------------------------------------------------------
// Example 2: Thread pool via io_context
//
// io_context::run() is thread-safe when called from multiple threads.
// Each posted task is executed by whichever thread picks it up first.
// ---------------------------------------------------------------------------
void thread_pool_example() {
    asio::io_context io;

    // work_guard prevents io.run() from returning while work is pending.
    auto work = asio::make_work_guard(io);

    // Launch a pool of 4 worker threads.
    std::vector<std::thread> pool;
    for (int i = 0; i < 4; ++i) {
        pool.emplace_back([&io] { io.run(); });
    }

    // Post 8 independent tasks to the pool.
    for (int i = 0; i < 8; ++i) {
        asio::post(io, [i] {
            std::cout << "[Pool] task " << i
                      << " on thread " << std::this_thread::get_id() << "\n";
        });
    }

    // Release the work guard so io.run() can return once queued tasks finish.
    work.reset();
    for (auto& t : pool) t.join();
}

// ---------------------------------------------------------------------------
// Example 3: Strand — serialise handlers across threads without a mutex
//
// Two "producers" post handlers to the same strand; the strand guarantees
// they never run concurrently, even though the io_context runs on many threads.
// ---------------------------------------------------------------------------
void strand_example() {
    asio::io_context io;
    auto work = asio::make_work_guard(io);

    // A strand tied to the io_context.
    asio::strand<asio::io_context::executor_type> strand(io.get_executor());

    std::vector<std::thread> pool;
    for (int i = 0; i < 4; ++i) {
        pool.emplace_back([&io] { io.run(); });
    }

    int shared_counter = 0; // Safe to modify without a mutex via the strand.

    for (int i = 0; i < 10; ++i) {
        asio::post(strand, [&shared_counter, i] {
            // Handlers on the same strand are never concurrent.
            ++shared_counter;
            std::cout << "[Strand] handler " << i
                      << "  counter=" << shared_counter << "\n";
        });
    }

    work.reset();
    for (auto& t : pool) t.join();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== Single async timer ===\n";
    single_timer_example();

    std::cout << "\n=== Thread pool via io_context ===\n";
    thread_pool_example();

    std::cout << "\n=== Strand serialisation ===\n";
    strand_example();

    return 0;
}
