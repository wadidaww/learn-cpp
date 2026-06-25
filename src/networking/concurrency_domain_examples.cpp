#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>
#include <vector>

namespace {

struct MarketOrder {
    std::uint32_t symbol_id{};
    std::uint32_t quantity{};
    std::uint64_t price_cents{};
};

struct GameInput {
    std::uint32_t player_id{};
    std::uint16_t sequence{};
    std::int16_t move_x{};
    std::int16_t move_y{};
};

struct TelemetrySample {
    std::uint32_t sensor_id{};
    std::uint64_t timestamp_ns{};
    float value{};
};

struct RpcRequest {
    std::uint32_t method_id{};
    std::uint32_t correlation_id{};
    std::array<char, 16> payload{};
};

struct ExchangeGatewayMessage {
    std::uint32_t venue_id{};
    std::uint32_t internal_symbol_id{};
    std::uint32_t external_symbol_id{};
    std::uint8_t side{};
};

template <typename T, std::size_t Capacity>
class BoundedQueue {
public:
    bool push(const T& value) {
        if (size_ == Capacity) {
            return false;
        }
        storage_[(head_ + size_) % Capacity] = value;
        ++size_;
        return true;
    }

    std::optional<T> pop() {
        if (size_ == 0) {
            return std::nullopt;
        }
        T value = storage_[head_];
        head_ = (head_ + 1) % Capacity;
        --size_;
        return value;
    }

    std::size_t size() const { return size_; }

private:
    std::array<T, Capacity> storage_{};
    std::size_t head_{};
    std::size_t size_{};
};

class OrderPool {
public:
    MarketOrder* acquire() {
        if (free_ == 0) {
            return nullptr;
        }
        return &orders_[--free_];
    }

    void release(MarketOrder* order) {
        if (order != nullptr && free_ < orders_.size()) {
            orders_[free_++] = *order;
        }
    }

private:
    std::array<MarketOrder, 8> orders_{};
    std::size_t free_{orders_.size()};
};

std::size_t worker_for_key(std::uint32_t key, std::size_t worker_count) {
    return key % worker_count;
}

std::uint64_t percentile(std::vector<std::uint64_t> values, double pct) {
    std::sort(values.begin(), values.end());
    const auto index = static_cast<std::size_t>((pct / 100.0) * (values.size() - 1));
    return values[index];
}

void demonstrate_domains() {
    const MarketOrder order{3, 100, 25'010};
    const GameInput input{9, 41, 1, -1};
    const TelemetrySample sample{12, 900'000, 42.5F};
    const RpcRequest rpc{7, 99, {}};
    const ExchangeGatewayMessage gateway{2, 3, 7003, 1};

    std::cout << "domains: trading symbol " << order.symbol_id
              << ", game player " << input.player_id
              << ", telemetry sensor " << sample.sensor_id
              << ", rpc method " << rpc.method_id
              << ", exchange venue " << gateway.venue_id << '\n';
}

void demonstrate_ownership_and_backpressure() {
    BoundedQueue<MarketOrder, 2> handoff;
    handoff.push({1, 10, 100});
    handoff.push({2, 20, 200});

    if (!handoff.push({3, 30, 300})) {
        std::cout << "backpressure: bounded handoff full at " << handoff.size() << " messages\n";
    }

    const auto worker = worker_for_key(42, 4);
    std::cout << "sharding: key 42 belongs to worker " << worker << '\n';
}

void demonstrate_pool_and_timing() {
    OrderPool pool;
    MarketOrder* order = pool.acquire();
    if (order != nullptr) {
        *order = {5, 50, 5050};
    }

    using Clock = std::chrono::steady_clock;
    std::vector<std::uint64_t> samples;
    samples.reserve(5);
    for (int i = 0; i < 5; ++i) {
        const auto start = Clock::now();
        std::this_thread::yield();
        const auto end = Clock::now();
        samples.push_back(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()));
    }

    std::cout << "pool/timing: reused order storage, p50 yield cost "
              << percentile(samples, 50.0) << " ns\n";
    pool.release(order);
}

} // namespace

int main() {
    demonstrate_domains();
    demonstrate_ownership_and_backpressure();
    demonstrate_pool_and_timing();
}
