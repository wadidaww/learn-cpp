#include <array>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "solution.hpp"

using namespace ll::itch;

namespace {

std::vector<std::byte> bytes(std::initializer_list<int> xs) {
    std::vector<std::byte> v;
    for (int x : xs) v.push_back(static_cast<std::byte>(x));
    return v;
}

// A hand-assembled Add Order:
// locate=0x0102, tracking=0x0304, ts=0x0000AABBCCDDEEFF -> 6 bytes AA BB CC DD EE FF,
// ref=0x1122334455667788, side='S', shares=100 (0x64), stock="AAPL    ", price=1890000 (0x001CD6D0)
std::vector<std::byte> sample() {
    return bytes({'A',  0x01, 0x02, 0x03, 0x04, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                  0x88, 'S',  0x00, 0x00, 0x00, 0x64, 'A',  'A',  'P',  'L',  ' ',  ' ',  ' ',  ' ',  0x00, 0x1C, 0xD6, 0xD0});
}

}  // namespace

TEST(load_be_basic) {
    auto b = bytes({0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0});
    CHECK_EQ(load_be<std::uint16_t>(b.data()), 0x1234u);
    CHECK_EQ(load_be<std::uint32_t>(b.data()), 0x12345678u);
    CHECK_EQ(load_be<std::uint64_t>(b.data()), 0x123456789ABCDEF0ull);
    CHECK_EQ(load_be48(b.data()), 0x123456789ABCull);
}

TEST(store_be_roundtrip) {
    std::byte buf[8]{};
    store_be<std::uint32_t>(buf, 0xDEADBEEFu);
    CHECK(buf[0] == std::byte{0xDE});
    CHECK(buf[3] == std::byte{0xEF});
    CHECK_EQ(load_be<std::uint32_t>(buf), 0xDEADBEEFu);
    store_be<std::uint64_t>(buf, 0x0102030405060708ull);
    CHECK(buf[0] == std::byte{0x01});
    CHECK(buf[7] == std::byte{0x08});
    std::byte six[6]{};
    store_be48(six, 0xFFFF123456789ABCull);  // high 16 bits dropped
    CHECK_EQ(load_be48(six), 0x123456789ABCull);
}

TEST(unaligned_loads_are_fine) {
    // Under UBSan a reinterpret_cast<const uint64_t*> shortcut fails here.
    std::vector<std::byte> b(32);
    for (std::size_t off = 0; off < 8; ++off) {
        store_be<std::uint64_t>(b.data() + off, 0xA1B2C3D4E5F60718ull + off);
        CHECK_EQ(load_be<std::uint64_t>(b.data() + off), 0xA1B2C3D4E5F60718ull + off);
        store_be<std::uint32_t>(b.data() + off, 0x01020304u);
        CHECK_EQ(load_be<std::uint32_t>(b.data() + off), 0x01020304u);
    }
}

TEST(decode_known_message) {
    auto b = sample();
    CHECK_EQ(b.size(), kAddOrderSize);
    AddOrder m{};
    CHECK(decode_add_order(b, m) == DecodeError::ok);
    CHECK_EQ(m.stock_locate, 0x0102);
    CHECK_EQ(m.tracking_number, 0x0304);
    CHECK_EQ(m.timestamp_ns, 0xAABBCCDDEEFFull);
    CHECK_EQ(m.order_ref, 0x1122334455667788ull);
    CHECK_EQ(m.side, 'S');
    CHECK_EQ(m.shares, 100u);
    CHECK(std::memcmp(m.stock.data(), "AAPL    ", 8) == 0);
    CHECK_EQ(m.price, 1890000u);
}

TEST(decode_from_odd_offset) {
    auto s = sample();
    std::array<std::byte, kAddOrderSize + 3> b{};
    std::memcpy(b.data() + 3, s.data(), kAddOrderSize);
    AddOrder m{};
    CHECK(decode_add_order(std::span(b).subspan(3), m) == DecodeError::ok);
    CHECK_EQ(m.order_ref, 0x1122334455667788ull);
    CHECK_EQ(m.price, 1890000u);
}

TEST(decode_errors) {
    auto b = sample();
    AddOrder m{};
    CHECK(decode_add_order(std::span(b).first(35), m) == DecodeError::too_short);
    CHECK(decode_add_order(std::span<const std::byte>{}, m) == DecodeError::too_short);
    b[0] = std::byte{'E'};
    CHECK(decode_add_order(b, m) == DecodeError::wrong_type);
    b = sample();
    b[19] = std::byte{'X'};
    CHECK(decode_add_order(b, m) == DecodeError::bad_side);
}

TEST(encode_matches_wire_bytes) {
    AddOrder m{};
    m.stock_locate = 0x0102;
    m.tracking_number = 0x0304;
    m.timestamp_ns = 0xAABBCCDDEEFFull;
    m.order_ref = 0x1122334455667788ull;
    m.side = 'S';
    m.shares = 100;
    std::memcpy(m.stock.data(), "AAPL    ", 8);
    m.price = 1890000;
    std::vector<std::byte> out(64);
    CHECK_EQ(encode_add_order(m, out), kAddOrderSize);
    auto want = sample();
    CHECK(std::memcmp(out.data(), want.data(), kAddOrderSize) == 0);
    std::vector<std::byte> small(35);
    CHECK_EQ(encode_add_order(m, small), 0u);
}

TEST(roundtrip_many) {
    std::vector<std::byte> buf(kAddOrderSize);
    for (std::uint64_t i = 0; i < 1000; ++i) {
        AddOrder a{};
        a.stock_locate = static_cast<std::uint16_t>(i * 7);
        a.tracking_number = static_cast<std::uint16_t>(i * 13);
        a.timestamp_ns = (i * 0x9E3779B97F4A7C15ull) & 0xFFFFFFFFFFFFull;
        a.order_ref = i * 0x9E3779B97F4A7C15ull;
        a.side = (i & 1) ? 'B' : 'S';
        a.shares = static_cast<std::uint32_t>(i * 31);
        std::memcpy(a.stock.data(), "MSFT    ", 8);
        a.price = static_cast<std::uint32_t>(i * 104729);
        CHECK_EQ(encode_add_order(a, buf), kAddOrderSize);
        AddOrder b{};
        CHECK(decode_add_order(buf, b) == DecodeError::ok);
        CHECK_EQ(b.order_ref, a.order_ref);
        CHECK_EQ(b.timestamp_ns, a.timestamp_ns);
        CHECK_EQ(b.price, a.price);
        CHECK_EQ(b.side, a.side);
    }
}

int main() { return run_all_tests(); }
