/**
 * @file server.cpp
 * @brief Unix Domain Socket (UDS) server: accepts a client connection and
 *        exchanges messages over a SOCK_STREAM socket on the local filesystem.
 *
 * Unix Domain Sockets (UDS) provide **bidirectional, full-duplex** communication
 * between processes on the *same host*. They offer:
 *
 *  - The familiar BSD socket API (send/recv, read/write)
 *  - Higher throughput than TCP loopback (no IP/TCP overhead)
 *  - Credential passing (SO_PEERCRED) for authentication
 *  - Both stream (SOCK_STREAM) and datagram (SOCK_DGRAM) modes
 *  - File-descriptor passing (SCM_RIGHTS) – unique to UDS
 *
 * UDS are the preferred IPC mechanism when:
 *  - You need bidirectional communication between unrelated processes
 *  - You want a request/reply protocol
 *  - You may later replace local IPC with network IPC (just swap AF_UNIX → AF_INET)
 *
 * Best practices demonstrated:
 *  - Set SO_REUSEADDR to avoid "address already in use" on restart
 *  - Unlink the socket file before binding (stale sockets from prior crashes)
 *  - Use abstract namespace (leading '\0') on Linux to avoid filesystem leftovers
 *    (this example uses a real path instead for clarity)
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 server.cpp -o uds_server
 *
 * Usage:
 *   Terminal 1: ./uds_server
 *   Terminal 2: ./uds_client
 */

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static constexpr const char* SOCKET_PATH = "/tmp/ipc_demo.sock";
static constexpr int         BACKLOG     = 5;

int main() {
    // ── Create server socket ─────────────────────────────────────────────
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "[uds_server] socket() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    // Remove stale socket file from a previous crash
    unlink(SOCKET_PATH);

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        std::cerr << "[uds_server] bind() failed: " << std::strerror(errno) << '\n';
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        std::cerr << "[uds_server] listen() failed: " << std::strerror(errno) << '\n';
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    std::cout << "[uds_server] Listening on " << SOCKET_PATH << ". Waiting for client...\n";

    // ── Accept one client ────────────────────────────────────────────────
    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd == -1) {
        std::cerr << "[uds_server] accept() failed: " << std::strerror(errno) << '\n';
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }
    std::cout << "[uds_server] Client connected.\n";

    // ── Receive request ──────────────────────────────────────────────────
    std::array<char, 256> buf{};
    ssize_t bytes = recv(client_fd, buf.data(), buf.size() - 1, 0);
    if (bytes > 0) {
        buf[static_cast<std::size_t>(bytes)] = '\0';
        std::cout << "[uds_server] Request: " << buf.data() << '\n';
    }

    // ── Send response ────────────────────────────────────────────────────
    const std::string response = "Hello from UDS server! Request received and processed.";
    if (send(client_fd, response.c_str(), response.size(), 0) == -1) {
        std::cerr << "[uds_server] send() failed: " << std::strerror(errno) << '\n';
    } else {
        std::cout << "[uds_server] Response sent.\n";
    }

    // ── Cleanup ──────────────────────────────────────────────────────────
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    std::cout << "[uds_server] Done.\n";
    return 0;
}
