// Round-trip latency through your epoll server: server thread busy-polls (timeout 0) vs blocks (-1).
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <thread>
#include <vector>

#include "bench.hpp"
#include "solution.hpp"

namespace {

void run(const char* label, int timeout_ms) {
    ll::EpollEchoServer srv;
    std::uint16_t port = 0;
    if (!srv.listen(0, port)) {
        std::printf("listen failed\n");
        return;
    }
    std::atomic<bool> stop{false};
    std::thread th([&] {
        ll::bench::pin_to_cpu(1);
        while (!stop.load(std::memory_order_relaxed)) srv.poll(timeout_ms);
    });
    ll::bench::pin_to_cpu(0);
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons(port);
    ::connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof a);
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    std::vector<std::uint64_t> samples;
    char msg[64]{};
    for (int i = 0; i < 50'000; ++i) {
        auto t0 = ll::bench::now_cycles();
        ::send(fd, msg, sizeof msg, MSG_NOSIGNAL);
        std::size_t got = 0;
        while (got < sizeof msg) {
            auto k = ::recv(fd, msg + got, sizeof msg - got, 0);
            if (k <= 0) break;
            got += static_cast<std::size_t>(k);
        }
        if (i >= 5'000) samples.push_back(ll::bench::now_cycles() - t0);
    }
    ::close(fd);
    stop = true;
    th.join();
    ll::bench::report(label, std::move(samples));
}

}  // namespace

int main() {
    run("server busy-polls (0)", 0);
    run("server blocks (-1 ... 10ms)", 10);
    std::printf("Both include the client's own blocking recv. Try making the client spin with MSG_DONTWAIT too.\n");
}
