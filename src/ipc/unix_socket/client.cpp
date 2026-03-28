/**
 * @file client.cpp
 * @brief Unix Domain Socket (UDS) client: connects to the server, sends a
 *        request, and receives the response.
 *
 * See server.cpp for a full explanation of the UDS design.
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 client.cpp -o uds_client
 *
 * Usage:
 *   Terminal 1: ./uds_server   (start first)
 *   Terminal 2: ./uds_client
 */

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static constexpr const char* SOCKET_PATH = "/tmp/ipc_demo.sock";

int main() {
    // ── Create client socket ─────────────────────────────────────────────
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        std::cerr << "[uds_client] socket() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Retry connect briefly to allow the server to start
    bool connected = false;
    for (int attempt = 0; attempt < 20 && !connected; ++attempt) {
        if (connect(sock_fd, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) == 0) {
            connected = true;
        } else {
            usleep(100'000); // wait 100 ms
        }
    }

    if (!connected) {
        std::cerr << "[uds_client] connect() failed: " << std::strerror(errno)
                  << " (is the server running?)\n";
        close(sock_fd);
        return 1;
    }

    std::cout << "[uds_client] Connected to server.\n";

    // ── Send request ─────────────────────────────────────────────────────
    const std::string request = "Hello from UDS client!";
    if (send(sock_fd, request.c_str(), request.size(), 0) == -1) {
        std::cerr << "[uds_client] send() failed: " << std::strerror(errno) << '\n';
        close(sock_fd);
        return 1;
    }
    std::cout << "[uds_client] Sent: " << request << '\n';

    // ── Receive response ─────────────────────────────────────────────────
    std::array<char, 256> buf{};
    ssize_t bytes = recv(sock_fd, buf.data(), buf.size() - 1, 0);
    if (bytes > 0) {
        buf[static_cast<std::size_t>(bytes)] = '\0';
        std::cout << "[uds_client] Response: " << buf.data() << '\n';
    } else if (bytes == -1) {
        std::cerr << "[uds_client] recv() failed: " << std::strerror(errno) << '\n';
    }

    close(sock_fd);
    std::cout << "[uds_client] Done.\n";
    return 0;
}
