#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <arpa/inet.h>
#include <sys/uio.h>
#include <unistd.h>

namespace {

constexpr std::size_t kFrameHeaderBytes = sizeof(std::uint32_t);

class RingBuffer {
public:
    bool push(std::byte value) {
        if (size_ == bytes_.size()) {
            return false;
        }
        bytes_[(read_ + size_) % bytes_.size()] = value;
        ++size_;
        return true;
    }

    bool pop(std::byte& value) {
        if (size_ == 0) {
            return false;
        }
        value = bytes_[read_];
        read_ = (read_ + 1) % bytes_.size();
        --size_;
        return true;
    }

    std::size_t size() const { return size_; }

private:
    std::array<std::byte, 16> bytes_{};
    std::size_t read_{};
    std::size_t size_{};
};

std::vector<std::byte> make_frame(std::string_view payload) {
    std::vector<std::byte> frame;
    frame.resize(kFrameHeaderBytes + payload.size());

    const auto network_size = htonl(static_cast<std::uint32_t>(payload.size()));
    std::memcpy(frame.data(), &network_size, sizeof(network_size));
    std::memcpy(frame.data() + kFrameHeaderBytes, payload.data(), payload.size());
    return frame;
}

std::string_view parse_frame(std::span<const std::byte> frame) {
    if (frame.size() < kFrameHeaderBytes) {
        throw std::runtime_error("short frame header");
    }

    std::uint32_t network_size{};
    std::memcpy(&network_size, frame.data(), sizeof(network_size));
    const auto payload_size = ntohl(network_size);
    if (frame.size() - kFrameHeaderBytes < payload_size) {
        throw std::runtime_error("short frame payload");
    }

    const auto* payload = reinterpret_cast<const char*>(frame.data() + kFrameHeaderBytes);
    return {payload, payload_size};
}

void demonstrate_ring_buffer() {
    RingBuffer buffer;
    for (const char ch : std::string_view{"ABCD"}) {
        buffer.push(static_cast<std::byte>(ch));
    }

    std::byte value{};
    buffer.pop(value);
    std::cout << "ring buffer: consumed '" << static_cast<char>(value) << "', remaining "
              << buffer.size() << " bytes\n";
}

void demonstrate_framing_and_endian() {
    const auto frame = make_frame("ORDER:BUY:100");
    const auto payload = parse_frame(frame);
    std::cout << "framing/endian: payload='" << payload << "'\n";
}

void demonstrate_scatter_gather() {
    int pipe_fds[2]{};
    if (::pipe(pipe_fds) == -1) {
        throw std::runtime_error("pipe() failed");
    }

    const std::string_view header{"HDR"};
    const std::string_view body{"PAYLOAD"};
    std::array<iovec, 2> parts{{
        {const_cast<char*>(header.data()), header.size()},
        {const_cast<char*>(body.data()), body.size()},
    }};

    if (::writev(pipe_fds[1], parts.data(), static_cast<int>(parts.size())) == -1) {
        throw std::runtime_error("writev() failed");
    }

    std::array<char, 10> received{};
    if (::read(pipe_fds[0], received.data(), received.size()) == -1) {
        throw std::runtime_error("read() failed");
    }

    ::close(pipe_fds[0]);
    ::close(pipe_fds[1]);

    std::cout << "scatter/gather: wrote '" << std::string_view(received.data(), received.size())
              << "' with one writev call\n";
}

} // namespace

int main() {
    demonstrate_ring_buffer();
    demonstrate_framing_and_endian();
    demonstrate_scatter_gather();
}
