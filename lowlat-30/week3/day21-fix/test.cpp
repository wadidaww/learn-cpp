#include <cstdlib>
#include <new>
#include <string>

#include "check.hpp"
#include "solution.hpp"

using namespace ll::fix;

// Count heap allocations so the tests can prove encode/decode never allocate.
static long g_allocs = 0;
void* operator new(std::size_t n) {
    ++g_allocs;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

namespace {

// Test-side framer, written independently of your code: body uses '|' for SOH.
std::string frame(std::string body) {
    for (auto& c : body)
        if (c == '|') c = SOH;
    std::string msg = "8=FIX.4.4\x01" "9=" + std::to_string(body.size()) + "\x01" + body;
    unsigned sum = 0;
    for (char c : msg) sum += static_cast<unsigned char>(c);
    char tail[8];
    std::snprintf(tail, sizeof tail, "10=%03u\x01", sum % 256);
    return msg + tail;
}

NewOrderSingle sample() {
    NewOrderSingle m;
    m.sender_comp_id = "KRESNA";
    m.target_comp_id = "EXCH";
    m.msg_seq_num = 42;
    m.sending_time = "20261127-14:30:00.123";
    m.cl_ord_id = "ORD-0001";
    m.symbol = "AAPL";
    m.side = '1';
    m.order_qty = 100;
    m.ord_type = '2';
    m.price = 1'890'100;
    m.time_in_force = '0';
    return m;
}

const std::string kSampleBody =
    "35=D|49=KRESNA|56=EXCH|34=42|52=20261127-14:30:00.123|11=ORD-0001|55=AAPL|54=1|38=100|40=2|44=189.0100|59=0|";

bool same(const NewOrderSingle& a, const NewOrderSingle& b) {
    return a.sender_comp_id == b.sender_comp_id && a.target_comp_id == b.target_comp_id &&
           a.msg_seq_num == b.msg_seq_num && a.sending_time == b.sending_time && a.cl_ord_id == b.cl_ord_id &&
           a.symbol == b.symbol && a.side == b.side && a.order_qty == b.order_qty && a.ord_type == b.ord_type &&
           a.price == b.price && a.time_in_force == b.time_in_force;
}

Status decode_status(const std::string& s) {
    NewOrderSingle m;
    return decode_new_order(s, m).status;
}

}  // namespace

TEST(checksum_is_byte_sum_mod_256) {
    CHECK_EQ(checksum(""), 0);
    CHECK_EQ(checksum("A"), 65);
    std::string s(300, '\xff');
    CHECK_EQ(checksum(s), static_cast<std::uint8_t>((300u * 255u) % 256u));
}

TEST(encode_matches_spec_byte_for_byte) {
    char buf[256];
    auto n = encode_new_order(sample(), buf);
    CHECK(std::string(buf, n) == frame(kSampleBody));
}

TEST(encode_prices_and_market_orders) {
    char buf[256];
    auto m = sample();
    m.price = 5;  // 0.0005
    auto n = encode_new_order(m, buf);
    CHECK(std::string(buf, n).find("\x01" "44=0.0005\x01") != std::string::npos);
    m.ord_type = '1';
    m.time_in_force = '3';
    n = encode_new_order(m, buf);
    CHECK(std::string(buf, n) ==
          frame("35=D|49=KRESNA|56=EXCH|34=42|52=20261127-14:30:00.123|11=ORD-0001|55=AAPL|54=1|38=100|40=1|59=3|"));
    m.msg_seq_num = 18'446'744'073'709'551'615ull;
    n = encode_new_order(m, buf);
    CHECK(std::string(buf, n).find("\x01" "34=18446744073709551615\x01") != std::string::npos);
}

TEST(encode_refuses_small_buffer) {
    std::string full = frame(kSampleBody);
    char buf[256];
    for (std::size_t cap = 0; cap < full.size(); ++cap) CHECK_EQ(encode_new_order(sample(), std::span(buf, cap)), 0u);
    CHECK_EQ(encode_new_order(sample(), std::span(buf, full.size())), full.size());
    auto m = sample();
    m.order_qty = 0;
    CHECK_EQ(encode_new_order(m, buf), 0u);
    m = sample();
    m.price = 0;
    CHECK_EQ(encode_new_order(m, buf), 0u);
}

TEST(decode_round_trip_points_into_buffer) {
    std::string s = frame(kSampleBody);
    NewOrderSingle m;
    auto r = decode_new_order(s, m);
    CHECK(r.status == Status::ok);
    CHECK_EQ(r.consumed, s.size());
    CHECK(same(m, sample()));
    CHECK(m.symbol.data() >= s.data() && m.symbol.data() < s.data() + s.size());
}

TEST(decode_any_order_unknown_tags_optional_tif) {
    std::string s = frame("35=D|55=MSFT|11=X|9999=junk|54=2|40=2|44=410.5|38=7|52=T|34=9|49=A|56=B|");
    NewOrderSingle m;
    CHECK(decode_new_order(s, m).status == Status::ok);
    CHECK(m.symbol == "MSFT");
    CHECK(m.side == '2');
    CHECK_EQ(m.price, 4'105'000);
    CHECK_EQ(m.order_qty, 7);
    CHECK_EQ(m.msg_seq_num, 9u);
    CHECK(m.time_in_force == '0');
    CHECK(decode_status(frame("35=D|49=A|56=B|34=1|52=T|11=X|55=S|54=1|38=1|40=2|44=1|")) == Status::ok);
    CHECK(decode_status(frame("35=D|49=A|56=B|34=1|52=T|11=X|55=S|54=1|38=1|40=1|")) == Status::ok);
}

TEST(decode_every_prefix_is_incomplete) {
    std::string s = frame(kSampleBody);
    for (std::size_t cut = 0; cut < s.size(); ++cut) {
        NewOrderSingle m;
        auto r = decode_new_order(std::string_view(s).substr(0, cut), m);
        CHECK(r.status == Status::incomplete);
        CHECK_EQ(r.consumed, 0u);
    }
}

TEST(decode_stops_at_first_message) {
    std::string a = frame(kSampleBody);
    std::string b = frame("35=D|49=A|56=B|34=2|52=T|11=Y|55=S|54=1|38=1|40=1|");
    std::string both = a + b;
    NewOrderSingle m;
    auto r = decode_new_order(both, m);
    CHECK(r.status == Status::ok);
    CHECK_EQ(r.consumed, a.size());
    CHECK(m.cl_ord_id == "ORD-0001");
    r = decode_new_order(std::string_view(both).substr(r.consumed), m);
    CHECK(r.status == Status::ok);
    CHECK(m.cl_ord_id == "Y");
}

TEST(decode_rejects_bad_framing) {
    std::string good = frame(kSampleBody);
    std::string s = good;
    s[7] = '2';  // FIX.4.2
    CHECK(decode_status(s) == Status::bad_begin_string);
    CHECK(decode_status("XYZ" + good) == Status::bad_begin_string);

    s = good;
    s[s.size() - 2] = static_cast<char>(s[s.size() - 2] == '0' ? '1' : '0');  // last checksum digit
    CHECK(decode_status(s) == Status::bad_checksum);

    // body one byte longer than 9= says: tag 10 is not where it should be
    std::string body = kSampleBody;
    for (auto& c : body)
        if (c == '|') c = SOH;
    std::string wrong = "8=FIX.4.4\x01" "9=" + std::to_string(body.size() - 1) + "\x01" + body + "10=000\x01";
    CHECK(decode_status(wrong) == Status::bad_body_length);
    CHECK(decode_status("8=FIX.4.4\x01" "9=1x\x01" + body + "10=000\x01") == Status::bad_body_length);

    NewOrderSingle m;
    auto r = decode_new_order(s, m);
    CHECK_EQ(r.consumed, good.size());  // a framing-valid but bad message is still skippable
}

TEST(decode_rejects_bad_content) {
    CHECK(decode_status(frame("35=8|49=A|56=B|34=1|52=T|11=X|55=S|54=1|38=1|40=1|")) == Status::wrong_msg_type);
    CHECK(decode_status(frame("49=A|35=D|56=B|34=1|52=T|11=X|55=S|54=1|38=1|40=1|")) == Status::wrong_msg_type);
    CHECK(decode_status(frame("35=D|49=A|56=B|34=1|52=T|55=S|54=1|38=1|40=1|")) == Status::missing_field);
    CHECK(decode_status(frame("35=D|49=A|56=B|34=1|52=T|11=X|55=S|54=1|38=1|40=2|")) == Status::missing_field);
    CHECK(decode_status(frame("35=D|49=A|56=B|34=1|52=T|11=X|55=S|54=1|38=1x|40=1|")) == Status::bad_value);
    CHECK(decode_status(frame("35=D|49=A|56=B|34=1|52=T|11=X|55=S|54=12|38=1|40=1|")) == Status::bad_value);
    CHECK(decode_status(frame("35=D|49=A|56=B|34=1|52=T|11=X|55=S|54=1|38=1|40=2|44=1.23456|")) == Status::bad_value);
    CHECK(decode_status(frame("35=D|49=A|56=B|34=1|52=T|11=|55=S|54=1|38=1|40=1|")) == Status::bad_value);
    CHECK(decode_status(frame("35=D|49=A|56=B|34=|52=T|11=X|55=S|54=1|38=1|40=1|")) == Status::bad_value);
}

TEST(no_heap_allocation) {
    std::string s = frame(kSampleBody);  // allocates before we start counting
    char buf[256];
    NewOrderSingle m;
    long before = g_allocs;
    for (int i = 0; i < 1000; ++i) {
        auto n = encode_new_order(sample(), buf);
        auto r = decode_new_order(std::string_view(buf, n), m);
        CHECK(r.status == Status::ok);
    }
    CHECK_EQ(g_allocs - before, 0);
}

int main() { return run_all_tests(); }
