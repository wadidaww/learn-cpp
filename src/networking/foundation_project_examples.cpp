#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <poll.h>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

#include <fcntl.h>

#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#endif

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
    int fds[2]{};
    if (::pipe(fds) == -1) {
        throw std::runtime_error("pipe() failed");
    }
    return {FileDescriptor(fds[0]), FileDescriptor(fds[1])};
}

void set_nonblocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("fcntl(O_NONBLOCK) failed");
    }
}

void write_all(int fd, std::string_view bytes) {
    while (!bytes.empty()) {
        const ssize_t written = ::write(fd, bytes.data(), bytes.size());
        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("write() failed");
        }
        bytes.remove_prefix(static_cast<std::size_t>(written));
    }
}

std::size_t drain_until_eagain(int fd) {
    std::array<char, 16> buffer{};
    std::size_t total = 0;
    for (;;) {
        const ssize_t received = ::read(fd, buffer.data(), buffer.size());
        if (received > 0) {
            total += static_cast<std::size_t>(received);
            continue;
        }
        if (received == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return total;
        }
        if (received == -1 && errno == EINTR) {
            continue;
        }
        if (received == 0) {
            return total;
        }
        throw std::runtime_error("read() failed while draining");
    }
}

void demonstrate_tcp_ip_basics() {
    struct Endpoint {
        std::array<std::uint8_t, 4> ip{};
        std::uint16_t port{};
    };

    const Endpoint loopback{{127, 0, 0, 1}, 9000};
    constexpr std::size_t ethernet_mtu = 1500;
    constexpr std::size_t ipv4_udp_headers = 20 + 8;
    constexpr std::size_t max_udp_payload = ethernet_mtu - ipv4_udp_headers;
    const std::size_t telemetry_payload = 3200;
    const auto datagrams = (telemetry_payload + max_udp_payload - 1) / max_udp_payload;

    std::cout << "ip/ports/mtu: " << static_cast<int>(loopback.ip[0]) << '.'
              << static_cast<int>(loopback.ip[1]) << '.' << static_cast<int>(loopback.ip[2]) << '.'
              << static_cast<int>(loopback.ip[3]) << ':' << loopback.port << " needs "
              << datagrams << " UDP datagrams below a " << ethernet_mtu << " byte MTU\n";
}

void demonstrate_tcp_udp_loss_reorder_and_congestion() {
    struct Datagram {
        std::uint32_t sequence{};
        bool delivered{};
    };

    std::vector<Datagram> feed{{1, true}, {2, false}, {3, true}, {2, true}, {4, true}};
    std::uint32_t expected = 1;
    std::uint32_t retransmissions = 0;
    std::uint32_t reordered = 0;

    for (const auto packet : feed) {
        if (!packet.delivered) {
            ++retransmissions;
            continue;
        }
        if (packet.sequence != expected) {
            ++reordered;
        } else {
            ++expected;
        }
    }

    std::size_t congestion_window = 4;
    congestion_window /= 2; // A loss signal makes TCP-style congestion control slow down.

    std::cout << "loss/reorder/congestion: retransmissions=" << retransmissions
              << ", reordered_or_duplicate=" << reordered
              << ", congestion_window_after_loss=" << congestion_window << '\n';
}

void demonstrate_nagle_latency_and_throughput() {
    struct TcpWritePolicy {
        bool tcp_no_delay{};
        std::vector<std::string_view> writes{};

        std::size_t packets_sent() const {
            return tcp_no_delay ? writes.size() : 1;
        }
    };

    const TcpWritePolicy latency_first{true, {"B", "U", "Y"}};
    const TcpWritePolicy throughput_first{false, {"B", "U", "Y"}};

    const auto start = std::chrono::steady_clock::now();
    std::uint64_t checksum = 0;
    for (int i = 0; i < 10'000; ++i) {
        checksum += static_cast<std::uint64_t>(i);
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;

    std::cout << "nagle/latency/throughput: TCP_NODELAY sends " << latency_first.packets_sent()
              << " tiny writes immediately; batching sends " << throughput_first.packets_sent()
              << " packet, sample work took "
              << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()
              << " us with checksum " << checksum << '\n';
}

void demonstrate_sendmsg_recvmsg_and_socket_buffers() {
    int raw_fds[2]{};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, raw_fds) == -1) {
        throw std::runtime_error("socketpair() failed");
    }
    FileDescriptor sender(raw_fds[0]);
    FileDescriptor receiver(raw_fds[1]);

    int requested_buffer = 4096;
    if (::setsockopt(sender.get(), SOL_SOCKET, SO_SNDBUF, &requested_buffer, sizeof(requested_buffer)) == -1) {
        throw std::runtime_error("setsockopt(SO_SNDBUF) failed");
    }

    std::array<char, 4> header{{'H', 'D', 'R', ':'}};
    std::array<char, 5> body{{'Q', 'U', 'O', 'T', 'E'}};
    std::array<iovec, 2> outgoing{{
        {header.data(), header.size()},
        {body.data(), body.size()},
    }};
    msghdr send_message{};
    send_message.msg_iov = outgoing.data();
    send_message.msg_iovlen = outgoing.size();

    if (::sendmsg(sender.get(), &send_message, 0) == -1) {
        throw std::runtime_error("sendmsg() failed");
    }

    std::array<char, 4> received_header{};
    std::array<char, 5> received_body{};
    std::array<iovec, 2> incoming{{
        {received_header.data(), received_header.size()},
        {received_body.data(), received_body.size()},
    }};
    msghdr recv_message{};
    recv_message.msg_iov = incoming.data();
    recv_message.msg_iovlen = incoming.size();

    if (::recvmsg(receiver.get(), &recv_message, 0) == -1) {
        throw std::runtime_error("recvmsg() failed");
    }

    std::cout << "sendmsg/recvmsg/socket buffers: "
              << std::string_view(received_header.data(), received_header.size())
              << std::string_view(received_body.data(), received_body.size())
              << " via scatter/gather sockets\n";
}

void demonstrate_select_poll_and_epoll_triggers() {
    auto select_pipe = make_pipe();
    write_all(select_pipe[1].get(), "S");
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(select_pipe[0].get(), &read_set);
    timeval timeout{0, 100'000};
    const int select_ready = ::select(select_pipe[0].get() + 1, &read_set, nullptr, nullptr, &timeout);
    if (select_ready == -1) {
        throw std::runtime_error("select() failed");
    }
    std::array<char, 1> byte{};
    if (::read(select_pipe[0].get(), byte.data(), byte.size()) == -1) {
        throw std::runtime_error("read() after select failed");
    }

    auto poll_pipe = make_pipe();
    write_all(poll_pipe[1].get(), "P");
    pollfd poll_read{poll_pipe[0].get(), POLLIN, 0};
    const int poll_ready = ::poll(&poll_read, 1, 100);
    if (poll_ready == -1) {
        throw std::runtime_error("poll() failed");
    }
    if (::read(poll_pipe[0].get(), byte.data(), byte.size()) == -1) {
        throw std::runtime_error("read() after poll failed");
    }

    auto level_pipe = make_pipe();
    set_nonblocking(level_pipe[0].get());
    FileDescriptor epoll_level(::epoll_create1(EPOLL_CLOEXEC));
    epoll_event level_event{};
    level_event.events = EPOLLIN;
    level_event.data.fd = level_pipe[0].get();
    if (::epoll_ctl(epoll_level.get(), EPOLL_CTL_ADD, level_pipe[0].get(), &level_event) == -1) {
        throw std::runtime_error("epoll_ctl(level) failed");
    }
    write_all(level_pipe[1].get(), "LL");
    epoll_event ready{};
    const int first_level = ::epoll_wait(epoll_level.get(), &ready, 1, 100);
    const int repeated_level = ::epoll_wait(epoll_level.get(), &ready, 1, 100);
    if (first_level == -1 || repeated_level == -1) {
        throw std::runtime_error("epoll_wait(level) failed");
    }
    drain_until_eagain(level_pipe[0].get());

    auto edge_pipe = make_pipe();
    set_nonblocking(edge_pipe[0].get());
    FileDescriptor epoll_edge(::epoll_create1(EPOLL_CLOEXEC));
    epoll_event edge_event{};
    edge_event.events = EPOLLIN | EPOLLET;
    edge_event.data.fd = edge_pipe[0].get();
    if (::epoll_ctl(epoll_edge.get(), EPOLL_CTL_ADD, edge_pipe[0].get(), &edge_event) == -1) {
        throw std::runtime_error("epoll_ctl(edge) failed");
    }
    write_all(edge_pipe[1].get(), "EE");
    const int edge_ready = ::epoll_wait(epoll_edge.get(), &ready, 1, 100);
    if (edge_ready == -1) {
        throw std::runtime_error("epoll_wait(edge) failed");
    }
    const auto drained = drain_until_eagain(edge_pipe[0].get());

    std::cout << "select/poll/epoll: select=" << select_ready << ", poll=" << poll_ready
              << ", level repeats=" << repeated_level << ", edge drained=" << drained << " bytes\n";
}

class CursorBuffer {
public:
    bool append(std::string_view bytes) {
        if (write_ + bytes.size() > storage_.size()) {
            return false;
        }
        std::copy(bytes.begin(), bytes.end(), storage_.begin() + static_cast<std::ptrdiff_t>(write_));
        write_ += bytes.size();
        return true;
    }

    std::string_view consume(std::size_t count) {
        const auto available = std::min(count, write_ - read_);
        const char* start = storage_.data() + read_;
        read_ += available;
        return {start, available};
    }

    std::size_t readable() const { return write_ - read_; }

private:
    std::array<char, 32> storage_{};
    std::size_t read_{};
    std::size_t write_{};
};

void demonstrate_read_write_cursors_and_reuse() {
    CursorBuffer input;
    input.append("LEN=5:HELLO");
    const auto header = input.consume(6);

    std::string reusable;
    reusable.reserve(64);
    const auto capacity_before = reusable.capacity();
    reusable.assign("parsed without repeated growth");

    std::cout << "cursors/reuse: header='" << header << "', remaining=" << input.readable()
              << ", string capacity stable=" << (capacity_before == reusable.capacity()) << '\n';
}

std::vector<std::byte> encode_varint(std::uint32_t value) {
    std::vector<std::byte> encoded;
    do {
        std::uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        encoded.push_back(static_cast<std::byte>(byte));
    } while (value != 0);
    return encoded;
}

struct DecodeResult {
    std::optional<std::uint32_t> value;
    std::size_t consumed{};
};

DecodeResult decode_varint(std::span<const std::byte> bytes) {
    std::uint32_t value = 0;
    int shift = 0;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const auto byte = static_cast<std::uint8_t>(bytes[i]);
        value |= static_cast<std::uint32_t>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
            return {value, i + 1};
        }
        shift += 7;
    }
    return {std::nullopt, 0};
}

void demonstrate_serialization_choices() {
    struct FixedQuote {
        std::uint32_t symbol{};
        std::uint32_t price{};
        std::uint32_t quantity{};
    };

    const FixedQuote binary{42, 101'025, 7};
    const std::string text = "symbol=42 price=1010.25 quantity=7";
    const auto varint = encode_varint(binary.quantity);
    const auto decoded = decode_varint(varint);

    std::cout << "serialization: fixed=" << sizeof(binary) << " bytes, text=" << text.size()
              << " bytes, varint_quantity_bytes=" << decoded.consumed << '\n';
}

template <typename T, std::size_t Capacity>
class SpscQueue {
public:
    bool push(const T& value) {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto next = (tail + 1) % Capacity;
        if (next == head_.load(std::memory_order_acquire)) {
            return false;
        }
        storage_[tail] = value;
        tail_.store(next, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        const auto head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        T value = storage_[head];
        head_.store((head + 1) % Capacity, std::memory_order_release);
        return value;
    }

private:
    std::array<T, Capacity> storage_{};
    std::atomic<std::size_t> head_{};
    std::atomic<std::size_t> tail_{};
};

void demonstrate_concurrency_affinity_and_numa() {
    constexpr std::size_t workers = 4;
    const std::uint32_t symbol = 42;
    const auto owning_worker = symbol % workers;

    SpscQueue<std::uint32_t, 4> handoff;
    handoff.push(symbol);
    const auto handed_off = handoff.pop();

    bool pin_attempted = false;
#ifdef __linux__
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(0, &cpus);
    pin_attempted = ::pthread_setaffinity_np(::pthread_self(), sizeof(cpus), &cpus) == 0;
#endif

    struct WorkerPlacement {
        std::size_t worker{};
        std::size_t numa_node{};
    };
    const WorkerPlacement placement{owning_worker, owning_worker / 2};

    std::cout << "concurrency/affinity/numa: symbol shard=" << placement.worker
              << ", numa_hint=" << placement.numa_node
              << ", lock_free_handoff_value=" << handed_off.value_or(0)
              << ", pin_attempted=" << pin_attempted << '\n';
}

struct ClientQueue {
    std::array<std::string_view, 2> pending{};
    std::size_t size{};

    bool enqueue(std::string_view message) {
        if (size == pending.size()) {
            return false;
        }
        pending[size++] = message;
        return true;
    }
};

void demonstrate_broadcaster_and_backpressure() {
    std::array<ClientQueue, 3> clients{};
    std::size_t dropped = 0;
    for (std::string_view update : {"BID", "ASK", "TRADE"}) {
        for (auto& client : clients) {
            if (!client.enqueue(update)) {
                ++dropped;
            }
        }
    }

    std::cout << "broadcaster/backpressure: slow-client drops=" << dropped
              << ", first-client-queued=" << clients.front().size << '\n';
}

enum class ParseError {
    None,
    ShortMessage,
    BadType,
};

struct ParsedMessage {
    std::uint8_t type{};
    std::uint32_t value{};
};

struct ParseResult {
    ParsedMessage message{};
    ParseError error{ParseError::None};
};

ParseResult parse_hot_path(std::span<const std::byte> bytes) {
    if (bytes.size() < 5) {
        return {{}, ParseError::ShortMessage};
    }
    const auto type = static_cast<std::uint8_t>(bytes[0]);
    if (type > 3) {
        return {{}, ParseError::BadType};
    }
    std::uint32_t value{};
    std::memcpy(&value, bytes.data() + 1, sizeof(value));
    return {{type, value}, ParseError::None};
}

void demonstrate_expected_style_error_handling() {
    const std::array<std::byte, 5> bytes{std::byte{1}, std::byte{7}, std::byte{0}, std::byte{0}, std::byte{0}};
    const auto parsed = parse_hot_path(bytes);
    std::cout << "error handling: parsed_type=" << static_cast<int>(parsed.message.type)
              << ", error_code=" << static_cast<int>(parsed.error) << '\n';
}

std::uint64_t percentile(std::vector<std::uint64_t> values, double pct) {
    std::sort(values.begin(), values.end());
    const auto index = static_cast<std::size_t>((pct / 100.0) * (values.size() - 1));
    return values[index];
}

void demonstrate_latency_harness() {
    using Clock = std::chrono::steady_clock;
    std::vector<std::uint64_t> samples;
    samples.reserve(16);

    for (std::size_t size : {16U, 64U, 256U, 1024U}) {
        const auto start = Clock::now();
        std::vector<std::byte> packet(size);
        std::fill(packet.begin(), packet.end(), std::byte{0x2A});
        const auto after_kernel_like_copy = Clock::now();
        std::uint64_t application_sum = 0;
        for (const auto byte : packet) {
            application_sum += static_cast<std::uint8_t>(byte);
        }
        const auto end = Clock::now();
        samples.push_back(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()));
        std::cout << "kernel/app timing: size=" << size
                  << ", copy_ns="
                  << std::chrono::duration_cast<std::chrono::nanoseconds>(after_kernel_like_copy - start).count()
                  << ", app_sum=" << application_sum << '\n';
    }

    std::cout << "latency harness: p50=" << percentile(samples, 50.0)
              << " ns, p99=" << percentile(samples, 99.0)
              << " ns, then compare with TCP_NODELAY socket examples for tiny messages\n";
}

void demonstrate_pitfall_checks() {
    struct HotPathPolicy {
        bool uses_percentiles{true};
        bool uses_std_endl{false};
        bool logs_every_packet{false};
        bool allocates_per_packet{false};
        bool ignores_backpressure{false};
    };

    const HotPathPolicy policy{};
    std::cout << "pitfalls: percentiles=" << policy.uses_percentiles
              << ", std::endl_in_hot_path=" << policy.uses_std_endl
              << ", sync_logging=" << policy.logs_every_packet
              << ", per_packet_allocations=" << policy.allocates_per_packet
              << ", ignores_backpressure=" << policy.ignores_backpressure << '\n';
}

void demonstrate_tool_commands() {
    std::cout << "tools: inspect syscalls with 'strace -e send,recv,epoll_wait', CPU with 'perf', sockets with 'ss'\n";
}

void demonstrate_library_learning_path() {
    constexpr std::array<std::string_view, 5> libraries{
        "Boost.Asio/standalone Asio for async design",
        "libuv for event-loop patterns",
        "io_uring after sockets and epoll",
        "DPDK for later kernel-bypass study",
        "Wireshark for packet inspection",
    };
    std::cout << "libraries:";
    for (const auto library : libraries) {
        std::cout << ' ' << library << ';';
    }
    std::cout << '\n';
}

} // namespace

int main() {
    demonstrate_tcp_ip_basics();
    demonstrate_tcp_udp_loss_reorder_and_congestion();
    demonstrate_nagle_latency_and_throughput();
    demonstrate_sendmsg_recvmsg_and_socket_buffers();
    demonstrate_select_poll_and_epoll_triggers();
    demonstrate_read_write_cursors_and_reuse();
    demonstrate_serialization_choices();
    demonstrate_concurrency_affinity_and_numa();
    demonstrate_broadcaster_and_backpressure();
    demonstrate_expected_style_error_handling();
    demonstrate_latency_harness();
    demonstrate_pitfall_checks();
    demonstrate_tool_commands();
    demonstrate_library_learning_path();
}
