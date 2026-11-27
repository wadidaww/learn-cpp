// Day 16 exercise: single-threaded edge-triggered epoll echo server.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ll {

class EpollEchoServer {
public:
    EpollEchoServer();
    ~EpollEchoServer();
    EpollEchoServer(const EpollEchoServer&) = delete;
    EpollEchoServer& operator=(const EpollEchoServer&) = delete;

    // Listen on 127.0.0.1:port (0 = kernel picks; the chosen port goes to bound_port).
    bool listen(std::uint16_t port, std::uint16_t& bound_port);

    // One epoll_wait(timeout_ms) and handle every returned event. Returns events handled, -1 on error.
    int poll(int timeout_ms);

    // The tests call this from another thread while poll() runs: keep the count in a std::atomic.
    std::size_t clients() const noexcept;

private:
    // Suggested: int ep_ = -1, listen_fd_ = -1;
    // per-connection pending output: e.g. std::vector<std::vector<std::byte>> indexed by fd
    // (allocation on accept is fine; the steady-state echo path should not allocate when nothing is pending).
};

}  // namespace ll
