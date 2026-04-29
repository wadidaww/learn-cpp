// boost_asio_async.cpp — Advanced
//
// Async connection-pool manager combining:
//   • boost::asio::thread_pool          — multi-threaded executor (C++14 model)
//   • asio::strand<thread_pool::executor_type> — per-connection serialisation;
//     all Connection state mutations run under the strand, eliminating the
//     need for any explicit mutex inside the class
//   • asio::steady_timer                — idle-timeout per connection +
//     periodic heartbeat reporter on the pool
//   • asio::post / asio::dispatch       — differentiate "schedule later" vs
//     "run now if already on executor"
//   • asio::make_work_guard             — keep the pool alive during setup
//   • boost::system::error_code         — propagate errors through async chains
//   • std::enable_shared_from_this      — extend object lifetime into callbacks
//
// Design rules (common in production Asio code):
//   1. Every mutable member of Connection is only touched from its strand.
//   2. Public methods are callable from any thread; they forward onto the
//      strand, so callers never need to think about synchronisation.
//   3. Lifetime is shared_ptr-managed; every async op captures shared_from_this
//      so the object cannot be destroyed while a handler is in flight.
//
// Compile:
//   g++ -std=c++17 boost_asio_async.cpp -lboost_system -pthread -o boost_asio_async

#include <boost/asio.hpp>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace asio = boost::asio;
using error_code = boost::system::error_code;
using namespace std::chrono_literals;

// ============================================================================
//  Connection
//  A logical resource in the pool (represents a DB connection, HTTP keep-alive
//  socket, etc.).  All mutable state lives here; mutations are serialised
//  through the connection's strand so no lock is ever needed.
// ============================================================================
class Connection : public std::enable_shared_from_this<Connection> {
public:
    using Strand  = asio::strand<asio::thread_pool::executor_type>;
    using Handler = std::function<void(error_code, std::string)>;

    Connection(int id, asio::thread_pool& pool,
               std::chrono::milliseconds idle_ms)
        : id_(id)
        , strand_(asio::make_strand(pool))
        , idle_timer_(strand_)   // timer runs on the strand executor
        , idle_ms_(idle_ms)
    {}

    // ---- Thread-safe public interface --------------------------------------

    // Submit a request; CompletionHandler is called with (ec, reply) from
    // inside the strand (i.e. sequentially with all other handlers for this
    // connection, but potentially on any OS thread in the pool).
    void submit(std::string req, Handler handler) {
        asio::post(strand_,
            [self = shared_from_this(),
             r    = std::move(req),
             h    = std::move(handler)]() mutable {
                self->do_execute(std::move(r), std::move(h));
            });
    }

    // Graceful close: cancel timers and stop accepting new work.
    // dispatch() is used (not post()) so that if the caller is already
    // running on this strand the close is immediate, not deferred.
    void close() {
        asio::dispatch(strand_, [self = shared_from_this()] {
            if (self->closed_) return;
            self->closed_ = true;
            self->idle_timer_.cancel();
            std::cout << "  [conn " << self->id_ << "] closed"
                      << "  handled=" << self->total_handled_ << "\n";
        });
    }

    int    id()      const noexcept { return id_; }
    size_t handled() const noexcept {
        return total_handled_.load(std::memory_order_relaxed);
    }

private:
    // ---- Runs exclusively on the strand ------------------------------------

    void do_execute(std::string req, Handler handler) {
        if (closed_) {
            handler(asio::error::operation_aborted, {});
            return;
        }
        reset_idle_timer();
        ++total_handled_;

        // In a real server this is where async_write/async_read would chain.
        // We reply synchronously to keep the example self-contained.
        handler({}, "[conn " + std::to_string(id_) + "] echo: " + req);
    }

    void reset_idle_timer() {
        // Re-arming from within the strand is safe: expires_after() cancels
        // any outstanding wait before setting the new deadline.
        idle_timer_.expires_after(idle_ms_);
        idle_timer_.async_wait([self = shared_from_this()](error_code ec) {
            if (ec) return;   // operation_aborted = re-armed or closed
            std::cout << "  [conn " << self->id_ << "] idle timeout"
                      << "  handled=" << self->total_handled_ << "\n";
            // A real pool would mark the connection as recyclable here.
        });
    }

    int                        id_;
    Strand                     strand_;
    asio::steady_timer         idle_timer_;
    std::chrono::milliseconds  idle_ms_;
    bool                       closed_{false};
    std::atomic<size_t>        total_handled_{0};
};

// ============================================================================
//  ConnectionPool
//  Distributes work across N connections with round-robin scheduling.
//  A periodic heartbeat prints live per-connection stats.
// ============================================================================
class ConnectionPool {
public:
    ConnectionPool(size_t num_threads, size_t pool_size,
                   std::chrono::milliseconds idle_ms)
        : pool_(num_threads)
        , heartbeat_(pool_.get_executor())
        , rr_(0)
    {
        connections_.reserve(pool_size);
        for (size_t i = 0; i < pool_size; ++i)
            connections_.push_back(
                std::make_shared<Connection>(
                    static_cast<int>(i), pool_, idle_ms));
        arm_heartbeat();
    }

    // Submit a request using a default printer for the reply.
    void submit(std::string req,
                Connection::Handler handler = default_printer) {
        // Atomically grab the next slot; wrapping via modulo is lock-free.
        size_t idx =
            rr_.fetch_add(1, std::memory_order_relaxed) % connections_.size();
        connections_[idx]->submit(std::move(req), std::move(handler));
    }

    // Cancel the heartbeat, close all connections, and drain the thread pool.
    void shutdown() {
        heartbeat_.cancel();
        for (auto& c : connections_) c->close();
        pool_.join();
    }

    void dump_stats() const {
        size_t total = 0;
        for (auto& c : connections_) {
            size_t n = c->handled();
            std::cout << "    conn[" << c->id() << "] handled=" << n << "\n";
            total += n;
        }
        std::cout << "    total=" << total << "\n";
    }

private:
    static void default_printer(error_code ec, std::string reply) {
        if (!ec) std::cout << "  " << reply << "\n";
    }

    // Reschedule the heartbeat every second until cancel() is called.
    void arm_heartbeat() {
        heartbeat_.expires_after(1s);
        heartbeat_.async_wait([this](error_code ec) {
            if (ec) return;
            std::cout << "[heartbeat] live stats:\n";
            dump_stats();
            arm_heartbeat();
        });
    }

    asio::thread_pool                        pool_;
    std::vector<std::shared_ptr<Connection>> connections_;
    asio::steady_timer                       heartbeat_;
    std::atomic<size_t>                      rr_;
};

// ============================================================================
//  main
// ============================================================================
int main() {
    std::cout << "=== Async Connection Pool"
                 " (strand + idle-timeout + heartbeat) ===\n\n";

    // 4 OS threads, 3 logical connections, 300 ms idle timeout.
    ConnectionPool pool(4, 3, 300ms);

    constexpr int N = 24;
    std::atomic<int> done{0};

    for (int i = 0; i < N; ++i) {
        pool.submit(
            "request-" + std::to_string(i),
            [&done, i](error_code ec, std::string reply) {
                if (!ec)
                    std::cout << "  [" << i << "] " << reply << "\n";
                ++done;
            });
    }

    // Busy-wait until all replies arrive, then let idle timers fire.
    while (done.load() < N)
        std::this_thread::sleep_for(10ms);
    std::this_thread::sleep_for(400ms);

    pool.shutdown();

    std::cout << "\nFinal stats:\n";
    pool.dump_stats();
    return 0;
}
