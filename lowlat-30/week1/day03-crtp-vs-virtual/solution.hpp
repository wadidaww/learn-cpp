// Day 3 exercise. Implement each strategy three ways. Write the bodies yourself.
// Non-template functions defined in a header must be marked `inline`.
#pragma once

#include <concepts>
#include <cstdint>
#include <span>
#include <variant>

namespace ll {

struct Tick {
    std::int64_t bid;
    std::int64_t ask;
    std::int64_t bid_qty;
    std::int64_t ask_qty;
};

// ---------- 1. Virtual dispatch ----------
class IStrategy {
public:
    virtual ~IStrategy() = default;
    virtual std::int64_t on_tick(const Tick& t) = 0;
};

class VMidMomentum : public IStrategy {
public:
    std::int64_t on_tick(const Tick& t) override;

private:
    // your state
};

class VImbalance : public IStrategy {
public:
    std::int64_t on_tick(const Tick& t) override;
};

// ---------- 2. CRTP ----------
template <class Derived>
class StrategyBase {
public:
    std::int64_t on_tick(const Tick& t);  // forwards to Derived::on_tick_impl
};

class CMidMomentum : public StrategyBase<CMidMomentum> {
public:
    std::int64_t on_tick_impl(const Tick& t);

private:
    // your state
};

class CImbalance : public StrategyBase<CImbalance> {
public:
    std::int64_t on_tick_impl(const Tick& t);
};

// ---------- 3. Concept + variant ----------
template <class S>
concept Strategy = requires(S s, const Tick& t) {
    { s.on_tick(t) } -> std::same_as<std::int64_t>;
};

using AnyStrategy = std::variant<CMidMomentum, CImbalance>;

// Each returns the sum of on_tick() over all ticks.
std::int64_t run_virtual(IStrategy& s, std::span<const Tick> ticks);

template <Strategy S>
std::int64_t run_crtp(S& s, std::span<const Tick> ticks);

std::int64_t run_variant(AnyStrategy& s, std::span<const Tick> ticks);

}  // namespace ll
