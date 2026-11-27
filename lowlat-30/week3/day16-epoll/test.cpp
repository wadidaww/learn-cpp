#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <thread>
#include <vector>

#include "check.hpp"
#include "rng.hpp"
#include "solution.hpp"

using namespace ll;

namespace {

int connect_to(std::uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons(port);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof a) < 0) {
        ::close(fd);
        return -1;
    }
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    return fd;
}

bool send_all(int fd, const char* p, std::size_t n) {
    while (n) {
        auto k = ::send(fd, p, n, MSG_NOSIGNAL);
        if (k <= 0) return false;
        p += k;
        n -= static_cast<std::size_t>(k);
    }
    return true;
}

bool recv_exact(int fd, char* p, std::size_t n) {
    while (n) {
        auto k = ::recv(fd, p, n, 0);
        if (k <= 0) return false;
        p += k;
        n -= static_cast<std::size_t>(k);
    }
    return true;
}

struct Running {
    EpollEchoServer srv;
    std::uint16_t port = 0;
    std::atomic<bool> stop{false};
    std::thread th;
    bool ok = false;
    Running() {
        ok = srv.listen(0, port);
        th = std::thread([this] {
            while (!stop.load(std::memory_order_relaxed)) srv.poll(10);
        });
    }
    ~Running() {
        stop = true;
        th.join();
    }
};

}  // namespace

TEST(listen_picks_a_port) {
    EpollEchoServer s;
    std::uint16_t port = 0;
    CHECK(s.listen(0, port));
    CHECK(port != 0);
    CHECK_EQ(s.clients(), 0u);
    CHECK(s.poll(0) >= 0);
}

TEST(single_client_echo) {
    Running r;
    CHECK(r.ok);
    int fd = connect_to(r.port);
    CHECK(fd >= 0);
    const char msg[] = "hello epoll";
    CHECK(send_all(fd, msg, sizeof msg));
    char back[sizeof msg]{};
    CHECK(recv_exact(fd, back, sizeof back));
    CHECK(std::memcmp(msg, back, sizeof msg) == 0);
    ::close(fd);
}

TEST(many_clients_many_messages) {
    Running r;
    CHECK(r.ok);
    constexpr int kClients = 20;
    std::vector<std::thread> ts;
    std::atomic<int> good{0};
    for (int c = 0; c < kClients; ++c)
        ts.emplace_back([&, c] {
            int fd = connect_to(r.port);
            if (fd < 0) return;
            SplitMix64 g{static_cast<std::uint64_t>(c) + 1};
            bool ok = true;
            for (int m = 0; m < 100 && ok; ++m) {
                std::vector<char> out(1 + g.next_below(3000));
                for (auto& ch : out) ch = static_cast<char>(g.next());
                std::vector<char> in(out.size());
                ok = send_all(fd, out.data(), out.size()) && recv_exact(fd, in.data(), in.size()) && in == out;
            }
            ::close(fd);
            if (ok) good.fetch_add(1);
        });
    for (auto& t : ts) t.join();
    CHECK_EQ(good.load(), kClients);
}

TEST(big_burst_forces_partial_writes) {
    Running r;
    CHECK(r.ok);
    int fd = connect_to(r.port);
    CHECK(fd >= 0);
    // Send 8 MB before reading anything: the server's socket send buffer fills, so it must
    // keep the rest and finish with EPOLLOUT.
    std::vector<char> out(8 << 20);
    SplitMix64 g{99};
    for (auto& ch : out) ch = static_cast<char>(g.next());
    std::vector<char> in(out.size());
    std::thread reader([&] { recv_exact(fd, in.data(), in.size()); });
    CHECK(send_all(fd, out.data(), out.size()));
    reader.join();
    CHECK(in == out);
    ::close(fd);
}

TEST(disconnects_are_cleaned_up) {
    Running r;
    CHECK(r.ok);
    for (int i = 0; i < 50; ++i) {
        int fd = connect_to(r.port);
        char c = 'x';
        send_all(fd, &c, 1);
        recv_exact(fd, &c, 1);
        ::close(fd);
    }
    for (int i = 0; i < 100 && r.srv.clients() != 0; ++i) usleep(10'000);
    CHECK_EQ(r.srv.clients(), 0u);
}

int main() { return run_all_tests(); }
