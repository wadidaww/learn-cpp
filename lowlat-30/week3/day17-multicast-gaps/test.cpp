#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "check.hpp"
#include "rng.hpp"
#include "solution.hpp"

using namespace ll;

namespace {

struct Sink {
    std::vector<std::pair<std::uint64_t, std::string>> msgs;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> gaps;
    void on_message(std::uint64_t seq, std::span<const std::byte> p) {
        msgs.emplace_back(seq, std::string(reinterpret_cast<const char*>(p.data()), p.size()));
    }
    void on_gap(std::uint64_t a, std::uint64_t b) { gaps.emplace_back(a, b); }
    std::vector<std::uint64_t> seqs() const {
        std::vector<std::uint64_t> v;
        for (auto& m : msgs) v.push_back(m.first);
        return v;
    }
};

std::vector<std::byte> pl(std::uint64_t seq) {
    std::string s = "m" + std::to_string(seq);
    std::vector<std::byte> v(s.size());
    std::memcpy(v.data(), s.data(), s.size());
    return v;
}

using Arb = LineArbiter<8, 32, Sink>;

}  // namespace

TEST(in_order_delivery) {
    Sink s;
    Arb a(1, s);
    for (std::uint64_t q = 1; q <= 5; ++q) a.on_packet(q, pl(q));
    CHECK((s.seqs() == std::vector<std::uint64_t>{1, 2, 3, 4, 5}));
    CHECK(s.msgs[2].second == "m3");
    CHECK_EQ(a.next_expected(), 6u);
    CHECK(s.gaps.empty());
}

TEST(duplicates_from_the_other_line_are_dropped) {
    Sink s;
    Arb a(1, s);
    a.on_packet(1, pl(1));  // A
    a.on_packet(1, pl(1));  // B
    a.on_packet(2, pl(2));  // B first this time
    a.on_packet(2, pl(2));  // A
    CHECK((s.seqs() == std::vector<std::uint64_t>{1, 2}));
    CHECK_EQ(a.duplicates(), 2u);
}

TEST(reordering_inside_window_is_repaired) {
    Sink s;
    Arb a(10, s);
    a.on_packet(12, pl(12));
    a.on_packet(11, pl(11));
    a.on_packet(12, pl(12));  // duplicate of a buffered packet
    CHECK(s.msgs.empty());
    CHECK_EQ(a.buffered(), 2u);
    a.on_packet(10, pl(10));
    CHECK((s.seqs() == std::vector<std::uint64_t>{10, 11, 12}));
    CHECK(s.msgs[1].second == "m11");
    CHECK_EQ(a.next_expected(), 13u);
    CHECK_EQ(a.buffered(), 0u);
    CHECK_EQ(a.duplicates(), 1u);
}

TEST(timeout_declares_gap_before_first_buffered) {
    Sink s;
    Arb a(1, s);
    a.on_packet(1, pl(1));
    a.on_packet(4, pl(4));
    a.on_packet(5, pl(5));
    a.on_packet(7, pl(7));
    a.on_timeout();
    CHECK((s.gaps == std::vector<std::pair<std::uint64_t, std::uint64_t>>{{2, 3}}));
    CHECK((s.seqs() == std::vector<std::uint64_t>{1, 4, 5}));
    CHECK_EQ(a.next_expected(), 6u);
    a.on_timeout();
    CHECK(s.gaps.back() == (std::pair<std::uint64_t, std::uint64_t>{6, 6}));
    CHECK_EQ(a.next_expected(), 8u);
    a.on_timeout();  // nothing buffered: no-op
    CHECK_EQ(s.gaps.size(), 2u);
}

TEST(beyond_window_with_empty_buffer_jumps) {
    Sink s;
    Arb a(1, s);
    a.on_packet(100, pl(100));
    CHECK((s.gaps == std::vector<std::pair<std::uint64_t, std::uint64_t>>{{1, 99}}));
    CHECK((s.seqs() == std::vector<std::uint64_t>{100}));
    CHECK_EQ(a.next_expected(), 101u);
}

TEST(beyond_window_flushes_oldest_gaps_first) {
    Sink s;
    Arb a(1, s);  // window 8: holds 2..8 while waiting for 1
    a.on_packet(3, pl(3));
    a.on_packet(4, pl(4));
    a.on_packet(10, pl(10));  // 10 >= 1 + 8: declare gap [1,2], deliver 3,4, then 10 fits (expected 5)
    CHECK((s.gaps == std::vector<std::pair<std::uint64_t, std::uint64_t>>{{1, 2}}));
    CHECK((s.seqs() == std::vector<std::uint64_t>{3, 4}));
    CHECK_EQ(a.next_expected(), 5u);
    CHECK_EQ(a.buffered(), 1u);
    a.on_packet(30, pl(30));  // gap [5,9], deliver 10, then gap [11,29], deliver 30
    CHECK((s.gaps.back() == std::pair<std::uint64_t, std::uint64_t>{11, 29}));
    CHECK((s.seqs() == std::vector<std::uint64_t>{3, 4, 10, 30}));
    CHECK_EQ(a.next_expected(), 31u);
}

TEST(randomised_two_lines) {
    // Each line drops 20% and reorders a little; everything carried by at least one line must arrive.
    SplitMix64 g{5};
    Sink s;
    LineArbiter<64, 16, Sink> a(1, s);
    constexpr std::uint64_t N = 20'000;
    std::set<std::uint64_t> carried;
    std::vector<std::uint64_t> stream;
    for (std::uint64_t q = 1; q <= N; ++q) {
        bool ina = g.next_below(5) != 0, inb = g.next_below(5) != 0;
        if (ina) stream.push_back(q);
        if (inb) stream.push_back(q);
        if (ina || inb) carried.insert(q);
    }
    for (std::size_t i = 0; i + 1 < stream.size(); i += 2)
        if (g.next_below(10) == 0) std::swap(stream[i], stream[i + 1]);
    for (auto q : stream) a.on_packet(q, pl(q));
    while (a.buffered() > 0) a.on_timeout();  // the stream ended: give up on every open gap
    auto got = s.seqs();
    bool sorted = std::is_sorted(got.begin(), got.end()) && std::adjacent_find(got.begin(), got.end()) == got.end();
    CHECK(sorted);
    std::set<std::uint64_t> gs(got.begin(), got.end());
    CHECK(gs == carried);
    std::uint64_t lost = 0;
    for (auto [x, y] : s.gaps) lost += y - x + 1;
    CHECK_EQ(lost + got.size(), N - (N - a.next_expected() + 1));
}

TEST(real_multicast_a_b_lines) {
    const char* A = "239.255.10.1";
    const char* B = "239.255.10.2";
    const std::uint16_t port = 41234;
    int ra = mcast_open_receiver(A, port), rb = mcast_open_receiver(B, port), tx = mcast_open_sender();
    CHECK(ra >= 0);
    CHECK(rb >= 0);
    CHECK(tx >= 0);
    if (ra < 0 || rb < 0 || tx < 0) return;
    Sink s;
    LineArbiter<256, 64, Sink> arb(1, s);
    SplitMix64 g{11};
    constexpr std::uint64_t N = 2000;
    std::set<std::uint64_t> carried;
    std::array<std::byte, 64> buf;
    auto drain = [&](int fd) {
        for (;;) {
            auto n = udp_recv(fd, buf);
            if (n < 8) break;
            std::uint64_t seq;
            std::memcpy(&seq, buf.data(), 8);
            arb.on_packet(seq, std::span(buf).subspan(8, static_cast<std::size_t>(n) - 8));
        }
    };
    for (std::uint64_t q = 1; q <= N; ++q) {
        std::array<std::byte, 24> pkt{};
        std::memcpy(pkt.data(), &q, 8);
        std::memcpy(pkt.data() + 8, "payload", 7);
        bool sa = g.next_below(10) != 0, sb = g.next_below(10) != 0;
        if (sa) CHECK(udp_send_to(tx, A, port, pkt));
        if (sb) CHECK(udp_send_to(tx, B, port, pkt));
        if (sa || sb) carried.insert(q);
        if (q % 16 == 0) {
            drain(ra);
            drain(rb);
        }
    }
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
    while (std::chrono::steady_clock::now() < deadline) {
        drain(ra);
        drain(rb);
    }
    while (arb.buffered() > 0) arb.on_timeout();
    auto got = s.seqs();
    std::set<std::uint64_t> gs(got.begin(), got.end());
    CHECK(gs == carried);
    CHECK(std::is_sorted(got.begin(), got.end()));
    ::close(ra);
    ::close(rb);
    ::close(tx);
}

int main() { return run_all_tests(); }
