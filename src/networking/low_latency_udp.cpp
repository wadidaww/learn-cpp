#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr std::string_view kLoopbackIp = "127.0.0.1";
constexpr int kServerPort = 9090;
constexpr int kIterations = 10'000;
constexpr int kSocketBufferBytes = 1 << 20;

// Low-latency lesson: keep messages small, fixed-size, and binary on the hot
// path. This avoids per-packet heap allocations and expensive text parsing.
struct Packet {
    std::uint64_t sequence{};
    std::uint64_t client_send_ns{};
    std::array<char, 32> payload{};
};

using Clock = std::chrono::steady_clock;

std::uint64_t now_ns() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            Clock::now().time_since_epoch())
            .count());
}

class UdpSocket {
public:
    UdpSocket() : fd_(::socket(AF_INET, SOCK_DGRAM, 0)) {
        if (fd_ == -1) {
            throw std::runtime_error("socket() failed: " + std::string(std::strerror(errno)));
        }
    }

    ~UdpSocket() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    int fd() const { return fd_; }

    void bind_to(int port) const {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<std::uint16_t>(port));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (::bind(fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
            throw std::runtime_error("bind() failed: " + std::string(std::strerror(errno)));
        }
    }

    void make_non_blocking() const {
        const int flags = ::fcntl(fd_, F_GETFL, 0);
        if (flags == -1 || ::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw std::runtime_error("fcntl(O_NONBLOCK) failed: " + std::string(std::strerror(errno)));
        }
    }

    void set_int_option(int level, int option, int value, std::string_view name) const {
        if (::setsockopt(fd_, level, option, &value, sizeof(value)) == -1) {
            throw std::runtime_error("setsockopt(" + std::string(name) + ") failed: " +
                                     std::string(std::strerror(errno)));
        }
    }

private:
    int fd_{};
};

sockaddr_in loopback_address(int port) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<std::uint16_t>(port));
    if (::inet_pton(AF_INET, kLoopbackIp.data(), &address.sin_addr) != 1) {
        throw std::runtime_error("inet_pton() failed");
    }
    return address;
}

bool would_block() {
    return errno == EAGAIN || errno == EWOULDBLOCK;
}

void configure_low_latency_udp(const UdpSocket& socket) {
    // Low-latency lesson: allow quick restarts during experiments. This is less
    // important for UDP than TCP TIME_WAIT, but useful for local demos.
    socket.set_int_option(SOL_SOCKET, SO_REUSEADDR, 1, "SO_REUSEADDR");

    // Low-latency lesson: make buffers large enough to absorb small bursts.
    // Huge buffers can hide overload and increase queueing delay, so tune with
    // real traffic instead of blindly choosing the largest possible value.
    socket.set_int_option(SOL_SOCKET, SO_RCVBUF, kSocketBufferBytes, "SO_RCVBUF");
    socket.set_int_option(SOL_SOCKET, SO_SNDBUF, kSocketBufferBytes, "SO_SNDBUF");

    // Low-latency lesson: non-blocking sockets let the program decide whether
    // to spin briefly, poll many sockets with epoll, or do other work.
    socket.make_non_blocking();

    // Low-latency lesson: IP_TOS can request low delay from the network. Many
    // networks ignore or rewrite it, so treat it as a hint, not a guarantee.
    socket.set_int_option(IPPROTO_IP, IP_TOS, IPTOS_LOWDELAY, "IP_TOS/IPTOS_LOWDELAY");
}

void echo_server(std::atomic<bool>& running) {
    UdpSocket server;
    configure_low_latency_udp(server);
    server.bind_to(kServerPort);

    Packet packet{};
    sockaddr_in client_address{};
    socklen_t client_length = sizeof(client_address);

    while (running.load(std::memory_order_relaxed)) {
        const ssize_t received = ::recvfrom(server.fd(), &packet, sizeof(packet), 0,
                                            reinterpret_cast<sockaddr*>(&client_address),
                                            &client_length);
        if (received == static_cast<ssize_t>(sizeof(packet))) {
            // Low-latency lesson: echo the same fixed-size packet immediately.
            // There is no allocation, logging, mutex, or formatting in this path.
            (void)::sendto(server.fd(), &packet, sizeof(packet), 0,
                           reinterpret_cast<sockaddr*>(&client_address), client_length);
        } else if (received == -1 && would_block()) {
            std::this_thread::yield();
        }
    }
}

std::uint64_t percentile(std::vector<std::uint64_t>& values, double pct) {
    const auto index = static_cast<std::size_t>((pct / 100.0) * (values.size() - 1));
    std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(index), values.end());
    return values[index];
}

} // namespace

int main() {
    try {
        std::atomic<bool> running{true};
        std::thread server([&running] { echo_server(running); });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        UdpSocket client;
        configure_low_latency_udp(client);
        const sockaddr_in server_address = loopback_address(kServerPort);

        // Low-latency lesson: reserve once before the benchmark starts. A vector
        // reallocation during packet processing would add noise to measurements.
        std::vector<std::uint64_t> round_trip_ns;
        round_trip_ns.reserve(kIterations);

        Packet packet{};
        std::copy_n("low-latency-demo", 16, packet.payload.begin());

        for (int i = 0; i < kIterations; ++i) {
            packet.sequence = static_cast<std::uint64_t>(i);
            packet.client_send_ns = now_ns();

            const ssize_t sent = ::sendto(client.fd(), &packet, sizeof(packet), 0,
                                          reinterpret_cast<const sockaddr*>(&server_address),
                                          sizeof(server_address));
            if (sent != static_cast<ssize_t>(sizeof(packet))) {
                throw std::runtime_error("sendto() failed: " + std::string(std::strerror(errno)));
            }

            Packet reply{};
            for (;;) {
                const ssize_t received = ::recvfrom(client.fd(), &reply, sizeof(reply), 0, nullptr, nullptr);
                if (received == static_cast<ssize_t>(sizeof(reply)) && reply.sequence == packet.sequence) {
                    round_trip_ns.push_back(now_ns() - reply.client_send_ns);
                    break;
                }

                if (received == -1 && would_block()) {
                    // Low-latency lesson: a short spin/yield can reduce wake-up
                    // latency on a dedicated CPU, but it burns CPU. Event loops
                    // such as epoll are usually better when many sockets share a
                    // thread or power usage matters.
                    std::this_thread::yield();
                    continue;
                }

                throw std::runtime_error("recvfrom() failed: " + std::string(std::strerror(errno)));
            }
        }

        running.store(false, std::memory_order_relaxed);
        server.join();

        auto values = round_trip_ns;
        const auto p50 = percentile(values, 50.0);
        values = round_trip_ns;
        const auto p99 = percentile(values, 99.0);
        values = round_trip_ns;
        const auto p999 = percentile(values, 99.9);

        std::cout << "UDP loopback round-trip latency over " << round_trip_ns.size() << " packets\n"
                  << "p50  = " << p50 << " ns\n"
                  << "p99  = " << p99 << " ns\n"
                  << "p99.9= " << p999 << " ns\n";
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }
}
