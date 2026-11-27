// Splits UDP loopback latency into (send -> kernel receive timestamp) and
// (kernel receive timestamp -> recv() returns), for a blocking and a busy-polling receiver.
// Uses SO_TIMESTAMPNS: the kernel stamps each packet with CLOCK_REALTIME when it is received.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstring>
#include <thread>
#include <vector>

#include "bench.hpp"

namespace {

std::int64_t realtime_ns() {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
}

void run(const char* label, bool busy) {
    int rx = ::socket(AF_INET, SOCK_DGRAM, 0);
    int one = 1;
    setsockopt(rx, SOL_SOCKET, SO_TIMESTAMPNS, &one, sizeof one);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = 0;
    ::bind(rx, reinterpret_cast<sockaddr*>(&a), sizeof a);
    socklen_t len = sizeof a;
    getsockname(rx, reinterpret_cast<sockaddr*>(&a), &len);

    constexpr int N = 20'000;
    std::vector<std::uint64_t> to_kernel, kernel_to_user;
    std::atomic<bool> ready{false};
    std::thread receiver([&] {
        ll::bench::pin_to_cpu(1);
        ready = true;
        for (int i = 0; i < N; ++i) {
            char buf[64];
            char ctrl[256];
            iovec iov{buf, sizeof buf};
            msghdr m{};
            m.msg_iov = &iov;
            m.msg_iovlen = 1;
            m.msg_control = ctrl;
            m.msg_controllen = sizeof ctrl;
            ssize_t n;
            do {
                n = ::recvmsg(rx, &m, busy ? MSG_DONTWAIT : 0);
            } while (n < 0 && (errno == EAGAIN || errno == EINTR));
            std::int64_t now = realtime_ns();
            std::int64_t sent = 0;
            std::memcpy(&sent, buf, sizeof sent);
            for (cmsghdr* c = CMSG_FIRSTHDR(&m); c; c = CMSG_NXTHDR(&m, c)) {
                if (c->cmsg_level == SOL_SOCKET && c->cmsg_type == SCM_TIMESTAMPNS) {
                    timespec ts;
                    std::memcpy(&ts, CMSG_DATA(c), sizeof ts);
                    std::int64_t k = static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
                    if (i >= N / 10 && k >= sent && now >= k) {
                        // stored as ns scaled to "cycles" so bench::report prints ns
                        double c2 = ll::bench::cycles_per_ns();
                        to_kernel.push_back(static_cast<std::uint64_t>(static_cast<double>(k - sent) * c2));
                        kernel_to_user.push_back(static_cast<std::uint64_t>(static_cast<double>(now - k) * c2));
                    }
                }
            }
        }
    });
    while (!ready) {
    }
    ll::bench::pin_to_cpu(0);
    int tx = ::socket(AF_INET, SOCK_DGRAM, 0);
    for (int i = 0; i < N; ++i) {
        char buf[64]{};
        std::int64_t t = realtime_ns();
        std::memcpy(buf, &t, sizeof t);
        ::sendto(tx, buf, sizeof buf, 0, reinterpret_cast<sockaddr*>(&a), sizeof a);
        auto until = realtime_ns() + 20'000;  // one packet every ~20 us so the receiver is idle when it arrives
        while (realtime_ns() < until) {
        }
    }
    receiver.join();
    ::close(tx);
    ::close(rx);
    std::printf("%s\n", label);
    ll::bench::report("  send -> kernel rx stamp", std::move(to_kernel));
    ll::bench::report("  kernel rx -> recv returns", std::move(kernel_to_user));
}

}  // namespace

int main() {
    (void)ll::bench::cycles_per_ns();
    run("blocking receiver", false);
    run("busy-polling receiver (MSG_DONTWAIT spin)", true);
    std::printf("\nThe second line is the wake-up + syscall + copy. Kernel bypass removes it (and most of the first).\n");
}
