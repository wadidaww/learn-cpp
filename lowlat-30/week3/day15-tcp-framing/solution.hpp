// Day 15 exercise. Non-template functions defined in this header must be `inline`.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <sys/types.h>

namespace ll {

// Frames: 2-byte big-endian payload length, then the payload.
template <std::size_t Cap>
class FrameDecoder {
    static_assert(Cap >= 3);

public:
    // Append received bytes. False if they do not fit (buffer full) or the decoder is in error.
    bool feed(std::span<const std::byte> bytes) noexcept;

    // If a complete frame is buffered, point `payload` at it and return true.
    // The span stays valid until the next feed(). Call consume() to release it.
    bool next(std::span<const std::byte>& payload) noexcept;
    void consume() noexcept;

    // A frame longer than Cap - 2 bytes was announced: the stream cannot be parsed further.
    bool error() const noexcept;

    std::size_t buffered() const noexcept;  // bytes held, including partial frames

private:
    // Suggested: std::array<std::byte, Cap> buf_; std::size_t begin_ = 0, end_ = 0; bool error_ = false;
};

// Returns bytes written (2 + payload.size()), or 0 if `out` is too small or payload > 65535 bytes.
std::size_t encode_frame(std::span<const std::byte> payload, std::span<std::byte> out) noexcept;

// ---- Socket helpers (IPv4 loopback is enough). Return -1 on error. ----
int tcp_listen(std::uint16_t port, std::uint16_t& bound_port);  // port 0: kernel picks
int tcp_accept(int listen_fd);
int tcp_connect(std::uint16_t port);  // connects to 127.0.0.1:port
bool set_nodelay(int fd, bool on);
bool send_all(int fd, std::span<const std::byte> data);       // loops until all sent; MSG_NOSIGNAL
ssize_t recv_some(int fd, std::span<std::byte> buf);            // one recv(); retries on EINTR

}  // namespace ll
