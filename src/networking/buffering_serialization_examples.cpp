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
    FileDescriptor read_fd(pipe_fds[0]);
    FileDescriptor write_fd(pipe_fds[1]);

    std::array<char, 3> header{{'H', 'D', 'R'}};
    std::array<char, 7> body{{'P', 'A', 'Y', 'L', 'O', 'A', 'D'}};
    std::array<iovec, 2> parts{{
        {header.data(), header.size()},
        {body.data(), body.size()},
    }};

    if (::writev(write_fd.get(), parts.data(), static_cast<int>(parts.size())) == -1) {
        throw std::runtime_error("writev() failed");
    }

    std::array<char, 10> received{};
    if (::read(read_fd.get(), received.data(), received.size()) == -1) {
        throw std::runtime_error("read() failed");
    }

    std::cout << "scatter/gather: wrote '" << std::string_view(received.data(), received.size())
              << "' with one writev call\n";
}

} // namespace

int main() {
    demonstrate_ring_buffer();
    demonstrate_framing_and_endian();
    demonstrate_scatter_gather();
}
