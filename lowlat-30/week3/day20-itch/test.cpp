#include <cstring>
#include <string>
#include <vector>

#include "check.hpp"
#include "solution.hpp"

using namespace ll::itch;

namespace {

// ---- Test-side encoder (independent of your parser) ----
struct W {
    std::vector<std::byte> b;
    void u8(std::uint8_t v) { b.push_back(std::byte{v}); }
    void ch(char c) { u8(static_cast<std::uint8_t>(c)); }
    void be(std::uint64_t v, int n) {
        for (int i = n - 1; i >= 0; --i) u8(static_cast<std::uint8_t>(v >> (8 * i)));
    }
    void hdr(char type, std::uint16_t loc, std::uint64_t ts) {
        ch(type);
        be(loc, 2);
        be(0, 2);
        be(ts, 6);
    }
    void text(const char* s, int n) {  // space-padded, stops at the terminator
        bool end = false;
        for (int i = 0; i < n; ++i) {
            end = end || s[i] == '\0';
            ch(end ? ' ' : s[i]);
        }
    }
};

struct Stream {
    std::vector<std::byte> bytes;
    void push(const W& m) {
        auto n = m.b.size();
        bytes.push_back(std::byte{static_cast<std::uint8_t>(n >> 8)});
        bytes.push_back(std::byte{static_cast<std::uint8_t>(n)});
        bytes.insert(bytes.end(), m.b.begin(), m.b.end());
    }
    void add(std::uint16_t loc, std::uint64_t ref, char side, std::uint32_t sh, std::uint32_t px, bool mpid = false) {
        W m;
        m.hdr(mpid ? 'F' : 'A', loc, 1000 + ref);
        m.be(ref, 8);
        m.ch(side);
        m.be(sh, 4);
        m.text("AAPL", 8);
        m.be(px, 4);
        if (mpid) m.text("GSCO", 4);
        push(m);
    }
    void exec(std::uint16_t loc, std::uint64_t ref, std::uint32_t sh, std::uint64_t match) {
        W m;
        m.hdr('E', loc, 2);
        m.be(ref, 8);
        m.be(sh, 4);
        m.be(match, 8);
        push(m);
    }
    void exec_px(std::uint16_t loc, std::uint64_t ref, std::uint32_t sh, std::uint64_t match, std::uint32_t px) {
        W m;
        m.hdr('C', loc, 3);
        m.be(ref, 8);
        m.be(sh, 4);
        m.be(match, 8);
        m.ch('Y');
        m.be(px, 4);
        push(m);
    }
    void cancel(std::uint16_t loc, std::uint64_t ref, std::uint32_t sh) {
        W m;
        m.hdr('X', loc, 4);
        m.be(ref, 8);
        m.be(sh, 4);
        push(m);
    }
    void del(std::uint16_t loc, std::uint64_t ref) {
        W m;
        m.hdr('D', loc, 5);
        m.be(ref, 8);
        push(m);
    }
    void replace(std::uint16_t loc, std::uint64_t o, std::uint64_t n, std::uint32_t sh, std::uint32_t px) {
        W m;
        m.hdr('U', loc, 6);
        m.be(o, 8);
        m.be(n, 8);
        m.be(sh, 4);
        m.be(px, 4);
        push(m);
    }
    void system_event() {
        W m;
        m.hdr('S', 0, 7);
        m.ch('O');
        push(m);
    }
};

struct Rec {
    std::vector<std::string> log;
    void on_add(const Header& h, std::uint64_t ref, char side, std::uint32_t sh, const std::array<char, 8>& st,
                std::uint32_t px) {
        log.push_back("A " + std::to_string(h.stock_locate) + " " + std::to_string(h.timestamp_ns) + " " +
                      std::to_string(ref) + " " + side + " " + std::to_string(sh) + " " + std::string(st.data(), 8) +
                      " " + std::to_string(px));
    }
    void on_executed(const Header&, std::uint64_t ref, std::uint32_t sh, std::uint64_t m) {
        log.push_back("E " + std::to_string(ref) + " " + std::to_string(sh) + " " + std::to_string(m));
    }
    void on_executed_with_price(const Header&, std::uint64_t ref, std::uint32_t sh, std::uint64_t m, std::uint32_t px) {
        log.push_back("C " + std::to_string(ref) + " " + std::to_string(sh) + " " + std::to_string(m) + " " +
                      std::to_string(px));
    }
    void on_cancel(const Header&, std::uint64_t ref, std::uint32_t sh) {
        log.push_back("X " + std::to_string(ref) + " " + std::to_string(sh));
    }
    void on_delete(const Header&, std::uint64_t ref) { log.push_back("D " + std::to_string(ref)); }
    void on_replace(const Header&, std::uint64_t o, std::uint64_t n, std::uint32_t sh, std::uint32_t px) {
        log.push_back("U " + std::to_string(o) + " " + std::to_string(n) + " " + std::to_string(sh) + " " +
                      std::to_string(px));
    }
};

}  // namespace

TEST(decodes_every_type) {
    Stream s;
    s.add(7, 1, 'B', 100, 1'890'000);
    s.add(7, 2, 'S', 50, 1'890'100, true);
    s.exec(7, 1, 30, 999);
    s.exec_px(7, 1, 10, 1000, 1'889'900);
    s.cancel(7, 2, 20);
    s.replace(7, 2, 3, 40, 1'890'200);
    s.del(7, 3);
    s.system_event();
    Rec r;
    ParseStats st;
    CHECK_EQ(parse_itch(s.bytes, r, st), s.bytes.size());
    CHECK_EQ(st.messages, 8u);
    CHECK_EQ(st.skipped, 1u);
    CHECK_EQ(st.errors, 0u);
    std::vector<std::string> want{"A 7 1001 1 B 100 AAPL     1890000", "A 7 1002 2 S 50 AAPL     1890100",
                                  "E 1 30 999",
                                  "C 1 10 1000 1889900",
                                  "X 2 20",
                                  "U 2 3 40 1890200",
                                  "D 3"};
    CHECK(r.log == want);
    if (r.log != want)
        for (auto& l : r.log) std::printf("    got: %s\n", l.c_str());
}

TEST(partial_trailing_message_is_left) {
    Stream s;
    s.add(1, 1, 'B', 1, 100);
    s.add(1, 2, 'B', 1, 100);
    auto full = s.bytes.size();
    for (std::size_t cut = 0; cut <= full; ++cut) {
        Rec r;
        ParseStats st;
        std::span<const std::byte> all(s.bytes);
        auto used = parse_itch(all.first(cut), r, st);
        std::size_t expect = cut < 38 ? 0 : (cut < 76 ? 38 : 76);
        CHECK_EQ(used, expect);
        // feed the rest after keeping the remainder, like a TCP/UDP reader would
        std::vector<std::byte> rest(s.bytes.begin() + static_cast<std::ptrdiff_t>(used), s.bytes.end());
        parse_itch(rest, r, st);
        CHECK_EQ(r.log.size(), 2u);
    }
}

TEST(wrong_length_counts_as_error_and_is_skipped) {
    Stream s;
    W bad;
    bad.hdr('D', 1, 0);
    bad.be(5, 8);
    bad.u8(0);  // 20 bytes instead of 19
    s.push(bad);
    s.del(1, 6);
    Rec r;
    ParseStats st;
    CHECK_EQ(parse_itch(s.bytes, r, st), s.bytes.size());
    CHECK_EQ(st.errors, 1u);
    CHECK((r.log == std::vector<std::string>{"D 6"}));
}

TEST(book_add_execute_cancel_delete) {
    Stream s;
    s.add(5, 1, 'B', 100, 1000);
    s.add(5, 2, 'B', 200, 1000);
    s.add(5, 3, 'B', 300, 990);
    s.add(5, 4, 'S', 50, 1010);
    s.add(9, 5, 'B', 999, 1005);  // other stock: ignored
    ItchBook b(5, 900, 1100, 1000);
    ParseStats st;
    parse_itch(s.bytes, b, st);
    CHECK_EQ(*b.best_bid(), 1000u);
    CHECK_EQ(*b.best_ask(), 1010u);
    CHECK_EQ(b.volume_at('B', 1000), 300u);
    CHECK_EQ(b.order_count(), 4u);

    Stream t;
    t.exec(5, 1, 100, 1);           // order 1 fully executed -> gone
    t.cancel(5, 2, 150);            // partial cancel -> 50 left
    t.exec_px(5, 2, 50, 2, 1000);   // rest of order 2 -> level 1000 empty
    t.del(5, 4);
    parse_itch(t.bytes, b, st);
    CHECK_EQ(*b.best_bid(), 990u);
    CHECK(!b.best_ask());
    CHECK_EQ(b.volume_at('B', 1000), 0u);
    CHECK_EQ(b.order_count(), 1u);
}

TEST(book_replace_moves_order) {
    Stream s;
    s.add(5, 10, 'S', 100, 1050);
    s.add(5, 11, 'S', 100, 1050);
    s.replace(5, 10, 12, 70, 1040);  // new ref, new price, new size, same side
    ItchBook b(5, 900, 1100, 1000);
    ParseStats st;
    parse_itch(s.bytes, b, st);
    CHECK_EQ(*b.best_ask(), 1040u);
    CHECK_EQ(b.volume_at('S', 1040), 70u);
    CHECK_EQ(b.volume_at('S', 1050), 100u);
    CHECK_EQ(b.order_count(), 2u);
    Stream t;
    t.del(5, 10);  // old ref no longer exists: ignore
    t.exec(5, 12, 70, 3);
    parse_itch(t.bytes, b, st);
    CHECK_EQ(*b.best_ask(), 1050u);
    CHECK_EQ(b.order_count(), 1u);
}

int main() { return run_all_tests(); }
