// Day 17 exercise: A/B line arbitration with a bounded reorder window, plus multicast sockets.
// Non-template functions defined in this header must be `inline`.
#pragma once

#include <sys/types.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace ll {

// Sink must provide:
//   void on_message(std::uint64_t seq, std::span<const std::byte> payload);
//   void on_gap(std::uint64_t first, std::uint64_t last);   // inclusive range that was lost
template <std::size_t W, std::size_t MaxPayload, class Sink>
class LineArbiter {
    static_assert(W >= 1);

public:
    LineArbiter(std::uint64_t first_expected, Sink& sink);

    // A packet from either line. Payloads longer than MaxPayload are truncated to MaxPayload.
    void on_packet(std::uint64_t seq, std::span<const std::byte> payload);

    // The caller's gap timer fired.
    void on_timeout();

    std::uint64_t next_expected() const noexcept;
    std::uint64_t duplicates() const noexcept;
    std::size_t buffered() const noexcept;

private:
    // Suggested: W slots { bool used; std::uint64_t seq; std::uint16_t len; std::array<std::byte, MaxPayload> data; }
};

// ---- Multicast sockets (IPv4). Return -1 on error. ----
int mcast_open_receiver(const char* group, std::uint16_t port);  // non-blocking, joined
int mcast_open_sender();                                         // TTL 1, IP_MULTICAST_LOOP on
bool udp_send_to(int fd, const char* group, std::uint16_t port, std::span<const std::byte> bytes);
ssize_t udp_recv(int fd, std::span<std::byte> buf);  // -1 with errno == EAGAIN when nothing is waiting

}  // namespace ll
