/**
 * @file sender.cpp
 * @brief IPC via File-Descriptor Passing (SCM_RIGHTS) — the sender side.
 *
 * ## What is FD Passing?
 *
 * Every open file descriptor is a kernel reference to an open-file description
 * (inode, position, flags).  Linux lets you *transmit* an open file descriptor
 * from one process to a completely unrelated process through a Unix Domain
 * Socket, using the `sendmsg()` syscall with *ancillary data* of type
 * `SCM_RIGHTS`.
 *
 * The receiver gets a brand-new file descriptor in its own descriptor table
 * that refers to the **same** open-file description as the one the sender
 * passed.  In other words:
 *
 *  - The two FDs share the same file-position pointer
 *  - They share the same access flags (read/write)
 *  - The underlying file stays open as long as *either* process has it
 *
 * This is fundamentally different from sharing a file path: the receiver needs
 * no knowledge of the filesystem location, and it inherits the exact state of
 * the open file (position, flags, inode).
 *
 * ## Typical use-cases
 *
 *  - Privilege separation: a privileged helper opens a restricted file and
 *    hands the FD to a sandboxed worker.
 *  - Zero-copy buffer hand-off: pass a memfd or pipe FD instead of copying data.
 *  - Socket hand-off: `systemd` passes pre-bound listen sockets to services.
 *
 * ## How ancillary data works
 *
 * `sendmsg()` and `recvmsg()` carry two orthogonal streams:
 *
 *   1. The *data* stream  – ordinary bytes (we send a short notification string)
 *   2. The *control* stream – ancillary messages (`struct cmsghdr`)
 *
 * An `SCM_RIGHTS` ancillary message embeds an array of `int` (file descriptors)
 * in the control buffer.  The kernel intercepts this and creates duplicate
 * descriptors in the receiver's process at the time of `recvmsg()`.
 *
 * Control buffer layout:
 * ┌─────────────────────────────────────────────────────┐
 * │ struct cmsghdr  { len, level=SOL_SOCKET, type=SCM_RIGHTS } │
 * │ int fd[N]                                           │
 * └─────────────────────────────────────────────────────┘
 *
 * Always use CMSG_SPACE / CMSG_LEN / CMSG_FIRSTHDR / CMSG_DATA macros —
 * never compute offsets manually, as alignment rules are platform-specific.
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 sender.cpp -o fd_sender
 *
 * Usage (two terminals):
 *   Terminal 1:  ./fd_receiver      ← start first
 *   Terminal 2:  ./fd_sender
 */

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static constexpr const char* SOCKET_PATH = "/tmp/ipc_fd_demo.sock";
static constexpr const char* TMP_FILE    = "/tmp/ipc_fd_demo_file.txt";

int main() {
    // ── Step 1: Create a temporary file and write content into it ────────
    int file_fd = open(TMP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (file_fd == -1) {
        std::cerr << "[fd_sender] open() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    const std::string content =
        "This content was written by fd_sender.\n"
        "The fd_receiver will read it via an fd passed over a Unix socket,\n"
        "without knowing the file path at all.\n";

    if (write(file_fd, content.c_str(), content.size()) == -1) {
        std::cerr << "[fd_sender] write() failed: " << std::strerror(errno) << '\n';
        close(file_fd);
        return 1;
    }

    // Rewind so the receiver reads from the beginning
    lseek(file_fd, 0, SEEK_SET);

    std::cout << "[fd_sender] Created " << TMP_FILE
              << " and wrote " << content.size() << " bytes.\n";

    // ── Step 2: Connect to the receiver's Unix Domain Socket ─────────────
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        std::cerr << "[fd_sender] socket() failed: " << std::strerror(errno) << '\n';
        close(file_fd);
        return 1;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Retry briefly so the user doesn't have to time the two programs exactly
    bool connected = false;
    for (int attempt = 0; attempt < 30 && !connected; ++attempt) {
        if (connect(sock_fd, reinterpret_cast<const struct sockaddr*>(&addr),
                    sizeof(addr)) == 0) {
            connected = true;
        } else {
            usleep(100'000); // 100 ms
        }
    }
    if (!connected) {
        std::cerr << "[fd_sender] connect() failed: " << std::strerror(errno)
                  << " (is fd_receiver running?)\n";
        close(file_fd);
        close(sock_fd);
        return 1;
    }
    std::cout << "[fd_sender] Connected to receiver.\n";

    // ── Step 3: Build sendmsg() with SCM_RIGHTS ancillary data ──────────
    //
    // The data iovec carries a short notification string so the receiver can
    // confirm the message frame arrived correctly.  The real payload is the
    // file descriptor in the ancillary control message.
    //
    // Control buffer: one cmsghdr + space for one int (the FD).
    // CMSG_SPACE accounts for the required padding/alignment.
    alignas(struct cmsghdr) std::array<char, CMSG_SPACE(sizeof(int))> ctrl_buf{};

    const std::string data_payload = "FD_TRANSFER";
    struct iovec iov{};
    iov.iov_base = const_cast<char*>(data_payload.c_str());
    iov.iov_len  = data_payload.size();

    struct msghdr msg{};
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = ctrl_buf.data();
    msg.msg_controllen = ctrl_buf.size();

    // Fill in the control message header
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(int));

    // Copy the file descriptor into the ancillary data payload
    std::memcpy(CMSG_DATA(cmsg), &file_fd, sizeof(file_fd));

    if (sendmsg(sock_fd, &msg, 0) == -1) {
        std::cerr << "[fd_sender] sendmsg() failed: " << std::strerror(errno) << '\n';
        close(file_fd);
        close(sock_fd);
        return 1;
    }

    std::cout << "[fd_sender] File descriptor (fd=" << file_fd
              << ") passed to receiver via SCM_RIGHTS.\n";

    // ── Cleanup ──────────────────────────────────────────────────────────
    // We close our copy; the receiver's duplicate keeps the file alive.
    close(file_fd);
    close(sock_fd);
    unlink(TMP_FILE);

    std::cout << "[fd_sender] Done (local fd closed; tmp file removed).\n";
    return 0;
}
