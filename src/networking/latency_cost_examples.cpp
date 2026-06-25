#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory_resource>
#include <mutex>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <sys/uio.h>
#include <unistd.h>

namespace {

class FileDescriptor {
public:
    explicit FileDescriptor(int fd = -1) : fd_(fd) {}
    ~FileDescriptor() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) {
            if (fd_ != -1) {
                ::close(fd_);
            }
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const { return fd_; }

private:
    int fd_;
};

std::array<FileDescriptor, 2> make_pipe() {
    int pipe_fds[2]{};
    if (::pipe(pipe_fds) == -1) {
        throw std::runtime_error("pipe() failed");
    }
    return {FileDescriptor(pipe_fds[0]), FileDescriptor(pipe_fds[1])};
}

void demonstrate_kernel_user_transitions() {
    auto pipe_fds = make_pipe();
    const std::array<std::string_view, 4> parts{"AB", "CD", "EF", "GH"};

    // Four writes mean four user/kernel transitions for one logical message.
    for (const auto part : parts) {
        if (::write(pipe_fds[1].get(), part.data(), part.size()) == -1) {
            throw std::runtime_error("write() failed");
        }
    }

    std::array<char, 8> separate_writes{};
    if (::read(pipe_fds[0].get(), separate_writes.data(), separate_writes.size()) == -1) {
        throw std::runtime_error("read() failed");
    }

    std::array<iovec, 4> iov{};
    for (std::size_t i = 0; i < parts.size(); ++i) {
        iov[i].iov_base = const_cast<char*>(parts[i].data());
        iov[i].iov_len = parts[i].size();
    }

    // writev moves the same four pieces with one syscall.
    if (::writev(pipe_fds[1].get(), iov.data(), static_cast<int>(iov.size())) == -1) {
        throw std::runtime_error("writev() failed");
    }

    std::array<char, 8> gathered{};
    if (::read(pipe_fds[0].get(), gathered.data(), gathered.size()) == -1) {
        throw std::runtime_error("read() failed");
    }

    std::cout << "kernel/user transitions: writev gathered '"
              << std::string_view(gathered.data(), gathered.size()) << "'\n";
}

struct WireQuote {
    std::uint32_t symbol_id{};
    std::uint32_t price_cents{};
    std::uint32_t quantity{};
};

WireQuote parse_quote_in_place(std::span<const std::byte, sizeof(WireQuote)> bytes) {
    WireQuote quote{};
    std::memcpy(&quote, bytes.data(), sizeof(quote));
    return quote;
}

void demonstrate_copies_and_allocations() {
    WireQuote quote{42, 125'050, 100};
    std::array<std::byte, sizeof(WireQuote)> packet{};
    std::memcpy(packet.data(), &quote, sizeof(quote));

    const WireQuote parsed = parse_quote_in_place(packet);

    std::array<std::byte, 4096> arena_storage{};
    std::pmr::monotonic_buffer_resource arena(arena_storage.data(), arena_storage.size());
    std::pmr::vector<WireQuote> reusable_messages{&arena};
    reusable_messages.reserve(8);
    reusable_messages.push_back(parsed);

    std::cout << "copies/allocations: parsed symbol " << reusable_messages.front().symbol_id
              << " with storage reserved before the hot path\n";
}

void demonstrate_locks_and_contention() {
    std::array<std::uint64_t, 4> shard_counts{};
    std::mutex metrics_mutex;

    const auto record_message = [&](std::uint32_t connection_id) {
        // Shard by owner first; only aggregate under a lock outside the per-packet path.
        ++shard_counts[connection_id % shard_counts.size()];
    };

    for (std::uint32_t connection = 0; connection < 16; ++connection) {
        record_message(connection);
    }

    std::uint64_t total{};
    {
        std::lock_guard lock(metrics_mutex);
        total = std::accumulate(shard_counts.begin(), shard_counts.end(), std::uint64_t{});
    }

    std::cout << "locks/contention: aggregated " << total << " messages after sharded updates\n";
}

void demonstrate_context_switches() {
    std::atomic<bool> ready{false};
    std::thread worker([&] {
        while (!ready.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    });

    ready.store(true, std::memory_order_release);
    worker.join();

    std::cout << "context switches: prefer event loops to one sleeping worker per socket\n";
}

void demonstrate_cache_misses() {
    struct FlatConnectionState {
        std::uint32_t connection_id{};
        std::uint32_t pending_bytes{};
    };

    std::vector<FlatConnectionState> states;
    states.reserve(8);
    for (std::uint32_t id = 0; id < 8; ++id) {
        states.push_back({id, id * 64});
    }

    const auto pending = std::accumulate(states.begin(), states.end(), std::uint32_t{},
                                         [](std::uint32_t sum, const FlatConnectionState& state) {
                                             return sum + state.pending_bytes;
                                         });
    std::cout << "cache locality: contiguous states have " << pending << " pending bytes\n";
}

class BoundedPacketQueue {
public:
    bool push(WireQuote quote) {
        if (size_ == storage_.size()) {
            return false;
        }
        storage_[(head_ + size_) % storage_.size()] = quote;
        ++size_;
        return true;
    }

    bool pop(WireQuote& quote) {
        if (size_ == 0) {
            return false;
        }
        quote = storage_[head_];
        head_ = (head_ + 1) % storage_.size();
        --size_;
        return true;
    }

    std::size_t size() const { return size_; }

private:
    std::array<WireQuote, 4> storage_{};
    std::size_t head_{};
    std::size_t size_{};
};

void demonstrate_delayed_packet_handling() {
    BoundedPacketQueue queue;
    for (std::uint32_t i = 0; i < 6; ++i) {
        if (!queue.push({i, 100 + i, 1})) {
            std::cout << "delayed packet handling: queue full, applying backpressure at depth "
                      << queue.size() << '\n';
            break;
        }
    }
}

void demonstrate_protocol_overhead() {
    const WireQuote binary{7, 101'025, 250};
    const std::string text = "symbol=7 price=1010.25 quantity=250";

    std::cout << "protocol overhead: binary quote is " << sizeof(binary)
              << " bytes, text form is " << text.size() << " bytes\n";
}

} // namespace

int main() {
    demonstrate_kernel_user_transitions();
    demonstrate_copies_and_allocations();
    demonstrate_locks_and_contention();
    demonstrate_context_switches();
    demonstrate_cache_misses();
    demonstrate_delayed_packet_handling();
    demonstrate_protocol_overhead();
}
