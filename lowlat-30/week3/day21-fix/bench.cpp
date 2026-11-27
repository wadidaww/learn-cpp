// Encode + decode cost of one NewOrderSingle. Usage: day21_bench [cpu=-1]
#include <cstdlib>
#include <vector>

#include "bench.hpp"
#include "solution.hpp"

int main(int argc, char** argv) {
    if (argc > 1 && !ll::bench::pin_to_cpu(std::atoi(argv[1]))) std::printf("warning: could not pin\n");
    ll::fix::NewOrderSingle m;
    m.sender_comp_id = "KRESNA";
    m.target_comp_id = "EXCH";
    m.sending_time = "20261127-14:30:00.123";
    m.cl_ord_id = "ORD-0001";
    m.symbol = "AAPL";
    m.order_qty = 100;
    m.price = 1'890'100;
    char buf[256];
    constexpr int kBatch = 100, kSamples = 20'000;
    std::vector<std::uint64_t> enc, dec;
    enc.reserve(kSamples);
    dec.reserve(kSamples);
    ll::fix::NewOrderSingle out;
    std::size_t n = 0;
    for (int s = 0; s < kSamples; ++s) {
        auto t0 = ll::bench::now_cycles();
        for (int i = 0; i < kBatch; ++i) {
            m.msg_seq_num = static_cast<std::uint64_t>(s * kBatch + i);
            n = ll::fix::encode_new_order(m, buf);
            ll::bench::clobber_memory();
        }
        auto t1 = ll::bench::now_cycles();
        for (int i = 0; i < kBatch; ++i) {
            auto r = ll::fix::decode_new_order(std::string_view(buf, n), out);
            ll::bench::do_not_optimize(r);
            ll::bench::clobber_memory();
        }
        auto t2 = ll::bench::now_cycles();
        enc.push_back((t1 - t0) / kBatch);
        dec.push_back((t2 - t1) / kBatch);
    }
    std::printf("message size: %zu bytes (per-call times below are batch averages of %d calls)\n", n, kBatch);
    ll::bench::report("encode", std::move(enc));
    ll::bench::report("decode", std::move(dec));
}
