// Day 4 exercise. Write every body yourself; pick the weakest correct memory order each time
// and be ready to justify it.
#pragma once

#include <atomic>
#include <cstdint>

namespace ll {

class TasSpinlock {
public:
    void lock() noexcept;
    bool try_lock() noexcept;
    void unlock() noexcept;

private:
    std::atomic<bool> locked_{false};
};

class TtasSpinlock {
public:
    void lock() noexcept;
    bool try_lock() noexcept;
    void unlock() noexcept;

private:
    std::atomic<bool> locked_{false};
};

// One producer thread, one consumer thread, one value in flight at a time.
class Mailbox {
public:
    // Returns false if the previous value has not been consumed yet.
    bool publish(std::int64_t value) noexcept;
    // Returns false if there is nothing to consume.
    bool try_consume(std::int64_t& out) noexcept;

private:
    std::int64_t payload_ = 0;  // deliberately NOT atomic
    std::atomic<bool> full_{false};
};

}  // namespace ll
