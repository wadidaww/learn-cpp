#include <unistd.h>

#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "check.hpp"
#include "solution.hpp"

using namespace ll;

namespace {

std::vector<std::byte> frame(const std::string& s) {
    std::vector<std::byte> out(s.size() + 2);
    auto n = encode_frame(std::as_bytes(std::span(s.data(), s.size())), out);
    out.resize(n);
    return out;
}

std::string str(std::span<const std::byte> b) { return {reinterpret_cast<const char*>(b.data()), b.size()}; }

}  // namespace

TEST(encode_header_is_big_endian) {
    std::string s(300, 'x');
    auto f = frame(s);
    CHECK_EQ(f.size(), 302u);
    CHECK(f[0] == std::byte{0x01});
    CHECK(f[1] == std::byte{0x2C});
    std::array<std::byte, 4> small{};
    CHECK_EQ(encode_frame(std::as_bytes(std::span(s.data(), s.size())), small), 0u);
}

TEST(one_frame_one_feed) {
    FrameDecoder<64> d;
    auto f = frame("hello");
    CHECK(d.feed(f));
    std::span<const std::byte> p;
    CHECK(d.next(p));
    CHECK(str(p) == "hello");
    d.consume();
    CHECK(!d.next(p));
    CHECK_EQ(d.buffered(), 0u);
}

TEST(empty_payload_frame) {
    FrameDecoder<16> d;
    auto f = frame("");
    CHECK(d.feed(f));
    std::span<const std::byte> p;
    CHECK(d.next(p));
    CHECK_EQ(p.size(), 0u);
    d.consume();
}

TEST(split_at_every_boundary) {
    std::vector<std::string> msgs{"A", "", "order:12345", std::string(200, 'z'), "end"};
    std::vector<std::byte> stream;
    for (auto& m : msgs) {
        auto f = frame(m);
        stream.insert(stream.end(), f.begin(), f.end());
    }
    for (std::size_t cut = 0; cut <= stream.size(); ++cut) {
        FrameDecoder<512> d;
        std::vector<std::string> got;
        std::span<const std::byte> all(stream);
        for (auto part : {all.first(cut), all.subspan(cut)}) {
            CHECK(d.feed(part));
            std::span<const std::byte> p;
            while (d.next(p)) {
                got.push_back(str(p));
                d.consume();
            }
        }
        CHECK(got == msgs);
    }
}

TEST(byte_at_a_time) {
    FrameDecoder<32> d;  // small buffer: must compact or it fills up
    std::vector<std::string> got;
    for (int round = 0; round < 50; ++round) {
        auto f = frame("msg" + std::to_string(round));
        for (auto b : f) {
            CHECK(d.feed(std::span(&b, 1)));
            std::span<const std::byte> p;
            while (d.next(p)) {
                got.push_back(str(p));
                d.consume();
            }
        }
    }
    CHECK_EQ(got.size(), 50u);
    CHECK(got.back() == "msg49");
}

TEST(oversized_length_is_an_error) {
    FrameDecoder<16> d;
    std::byte hdr[2]{std::byte{0x00}, std::byte{0x20}};  // 32 > 16 - 2
    CHECK(d.feed(hdr));
    std::span<const std::byte> p;
    CHECK(!d.next(p));
    CHECK(d.error());
    CHECK(!d.feed(hdr));
}

TEST(full_buffer_refuses) {
    FrameDecoder<8> d;
    std::byte junk[6]{std::byte{0}, std::byte{6}, std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    CHECK(d.feed(junk));  // partial frame of 6 announced, 4 received
    std::byte more[3]{};
    CHECK(!d.feed(more));  // 6 + 3 > 8
    CHECK(d.feed(std::span(more, 2)));
    std::span<const std::byte> p;
    CHECK(d.next(p));
    CHECK_EQ(p.size(), 6u);
}

TEST(loopback_echo_through_sockets) {
    std::uint16_t port = 0;
    int lfd = tcp_listen(0, port);
    CHECK(lfd >= 0);
    CHECK(port != 0);
    std::thread server([&] {
        int c = tcp_accept(lfd);
        std::array<std::byte, 4096> buf;
        for (;;) {
            auto n = recv_some(c, buf);
            if (n <= 0) break;
            send_all(c, std::span(buf).first(static_cast<std::size_t>(n)));
        }
        close(c);
    });
    int fd = tcp_connect(port);
    CHECK(fd >= 0);
    CHECK(set_nodelay(fd, true));
    std::vector<std::string> sent, got;
    FrameDecoder<1024> d;
    for (int i = 0; i < 200; ++i) {
        std::string m = "order " + std::to_string(i) + std::string(static_cast<std::size_t>(i % 37), '.');
        sent.push_back(m);
        CHECK(send_all(fd, frame(m)));
    }
    std::array<std::byte, 333> buf;  // odd size on purpose
    while (got.size() < sent.size()) {
        auto n = recv_some(fd, buf);
        if (n <= 0) break;
        CHECK(d.feed(std::span(buf).first(static_cast<std::size_t>(n))));
        std::span<const std::byte> p;
        while (d.next(p)) {
            got.push_back(str(p));
            d.consume();
        }
    }
    close(fd);
    server.join();
    close(lfd);
    CHECK(got == sent);
}

int main() { return run_all_tests(); }
