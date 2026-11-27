// Loopback TCP round trip, header and body sent as TWO writes (a common gateway bug),
// with Nagle on and off. With Nagle on, expect stalls of tens of milliseconds.
#include <unistd.h>

#include <thread>
#include <vector>

#include "bench.hpp"
#include "solution.hpp"

namespace {

void run(bool nodelay, int rounds) {
    std::uint16_t port = 0;
    int lfd = ll::tcp_listen(0, port);
    std::thread server([&] {
        int c = ll::tcp_accept(lfd);
        ll::set_nodelay(c, nodelay);
        ll::FrameDecoder<4096> d;
        std::array<std::byte, 4096> buf;
        for (int done = 0; done < rounds;) {
            auto n = ll::recv_some(c, buf);
            if (n <= 0) break;
            d.feed(std::span(buf).first(static_cast<std::size_t>(n)));
            std::span<const std::byte> p;
            while (d.next(p)) {
                std::array<std::byte, 64> out;
                auto k = ll::encode_frame(p, out);
                ll::send_all(c, std::span(out).first(k));  // reply in one write
                d.consume();
                ++done;
            }
        }
        close(c);
    });
    int fd = ll::tcp_connect(port);
    ll::set_nodelay(fd, nodelay);
    std::vector<std::uint64_t> samples;
    ll::FrameDecoder<4096> d;
    std::array<std::byte, 4096> buf;
    for (int i = 0; i < rounds; ++i) {
        std::array<std::byte, 32> body{};
        std::array<std::byte, 2> hdr{std::byte{0}, std::byte{32}};
        auto t0 = ll::bench::now_cycles();
        ll::send_all(fd, hdr);   // write 1: header
        ll::send_all(fd, body);  // write 2: body  <- Nagle holds this one
        std::span<const std::byte> p;
        while (!d.next(p)) {
            auto n = ll::recv_some(fd, buf);
            if (n <= 0) break;
            d.feed(std::span(buf).first(static_cast<std::size_t>(n)));
        }
        d.consume();
        samples.push_back(ll::bench::now_cycles() - t0);
    }
    close(fd);
    server.join();
    close(lfd);
    ll::bench::report(nodelay ? "TCP_NODELAY on" : "Nagle on (default)", std::move(samples));
}

}  // namespace

int main() {
    run(true, 20'000);
    run(false, 50);  // few rounds: each may take ~40 ms
    std::printf("\nNow change the client to send header+body in ONE write and rerun with Nagle on.\n");
}
