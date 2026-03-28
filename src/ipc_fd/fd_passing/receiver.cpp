/**
 * @file receiver.cpp
 * @brief IPC via File-Descriptor Passing (SCM_RIGHTS) — the receiver side.
 *
 * This program listens on a Unix Domain Socket, receives an open file
 * descriptor from the sender via `recvmsg()` + `SCM_RIGHTS` ancillary data,
 * and then reads the file through the received descriptor — without ever
 * knowing the file's path on the filesystem.
 *
 * See sender.cpp for a full explanation of the SCM_RIGHTS mechanism.
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 receiver.cpp -o fd_receiver
 *
 * Usage (two terminals):
 *   Terminal 1:  ./fd_receiver      ← start first
 *   Terminal 2:  ./fd_sender
 */

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static constexpr const char* SOCKET_PATH = "/tmp/ipc_fd_demo.sock";

int main() {
    // ── Step 1: Create the Unix Domain Socket server ─────────────────────
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "[fd_receiver] socket() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    unlink(SOCKET_PATH); // remove stale socket from previous run

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, reinterpret_cast<const struct sockaddr*>(&addr),
             sizeof(addr)) == -1) {
        std::cerr << "[fd_receiver] bind() failed: " << std::strerror(errno) << '\n';
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 1) == -1) {
        std::cerr << "[fd_receiver] listen() failed: " << std::strerror(errno) << '\n';
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    std::cout << "[fd_receiver] Listening on " << SOCKET_PATH
              << ". Waiting for sender...\n";

    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd == -1) {
        std::cerr << "[fd_receiver] accept() failed: " << std::strerror(errno) << '\n';
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }
    std::cout << "[fd_receiver] Sender connected.\n";

    // ── Step 2: Receive the message with ancillary SCM_RIGHTS data ───────
    //
    // The control buffer must be large enough for one cmsghdr + one int.
    // CMSG_SPACE adds the necessary platform alignment padding.
    alignas(struct cmsghdr) std::array<char, CMSG_SPACE(sizeof(int))> ctrl_buf{};

    std::array<char, 64> data_buf{};
    struct iovec iov{};
    iov.iov_base = data_buf.data();
    iov.iov_len  = data_buf.size() - 1;

    struct msghdr msg{};
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = ctrl_buf.data();
    msg.msg_controllen = ctrl_buf.size();

    ssize_t bytes = recvmsg(client_fd, &msg, 0);
    if (bytes == -1) {
        std::cerr << "[fd_receiver] recvmsg() failed: " << std::strerror(errno) << '\n';
        close(client_fd);
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    data_buf[static_cast<std::size_t>(bytes)] = '\0';
    std::cout << "[fd_receiver] Data frame received: \"" << data_buf.data() << "\"\n";

    // ── Step 3: Extract the file descriptor from ancillary data ──────────
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg == nullptr
        || cmsg->cmsg_level != SOL_SOCKET
        || cmsg->cmsg_type  != SCM_RIGHTS
        || cmsg->cmsg_len   != CMSG_LEN(sizeof(int))) {
        std::cerr << "[fd_receiver] No valid SCM_RIGHTS message found.\n";
        close(client_fd);
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    int received_fd = -1;
    std::memcpy(&received_fd, CMSG_DATA(cmsg), sizeof(received_fd));
    std::cout << "[fd_receiver] Received file descriptor: fd=" << received_fd << '\n';

    // ── Step 4: Read the file through the received descriptor ────────────
    //
    // Notice: we never called open() with a path.  We are reading data
    // from a file descriptor that was entirely created and named by the sender.
    std::cout << "[fd_receiver] Reading file content via received fd:\n"
              << "─────────────────────────────────────────────────────────\n";

    std::array<char, 512> file_buf{};
    ssize_t n = 0;
    while ((n = read(received_fd, file_buf.data(), file_buf.size() - 1)) > 0) {
        file_buf[static_cast<std::size_t>(n)] = '\0';
        std::cout << file_buf.data();
    }
    if (n == -1) {
        std::cerr << "[fd_receiver] read() error: " << std::strerror(errno) << '\n';
    }
    std::cout << "─────────────────────────────────────────────────────────\n";

    // ── Cleanup ──────────────────────────────────────────────────────────
    close(received_fd);
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    std::cout << "[fd_receiver] Done.\n";
    return 0;
}
