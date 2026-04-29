/**
 * @file select_example.cpp
 * @brief IPC multiplexing with select() — monitoring multiple pipe file
 *        descriptors from several child processes simultaneously.
 *
 * ## Why fd-multiplexing?
 *
 * When a server needs to read from N sources (pipes, sockets, regular files)
 * concurrently, the naive approach is one blocking thread per source.  That
 * approach does not scale and wastes kernel resources.
 *
 * The POSIX solution is to hand a *set* of file descriptors to the kernel and
 * ask it to wake you up as soon as *any* of them is ready.  Three APIs exist:
 *
 *  | API      | Complexity | Max FDs | Level/Edge | Platform |
 *  |----------|-----------|---------|-----------|---------|
 *  | select() | O(n)       | FD_SETSIZE (1024) | Level | POSIX   |
 *  | poll()   | O(n)       | unlimited | Level   | POSIX   |
 *  | epoll()  | O(1)*      | unlimited | Both    | Linux   |
 *
 *  *O(1) per ready event, not per registered FD.
 *
 * ## select() fundamentals
 *
 * ```c
 * int select(int nfds,
 *            fd_set *readfds,   // watch for readability
 *            fd_set *writefds,  // watch for writability
 *            fd_set *exceptfds, // watch for exceptions (OOB data)
 *            struct timeval *timeout); // NULL = block forever
 * ```
 *
 *  - `nfds` must be **max_fd + 1** (the kernel iterates 0..nfds-1)
 *  - `fd_set` is a **bit array** of up to FD_SETSIZE bits (usually 1024)
 *  - select() **modifies** the sets in-place — you must re-build them on
 *    every iteration
 *  - Returns the number of ready descriptors (0 = timeout, -1 = error)
 *
 * ## This example
 *
 * 1. Parent creates NUM_CHILDREN pipe pairs.
 * 2. Each child writes one message after a short (randomised) delay.
 * 3. Parent uses select() in a loop to read from whichever pipe becomes
 *    readable first, without blocking on any one child.
 * 4. Loop exits once all NUM_CHILDREN messages have been received.
 *
 * Best practices demonstrated:
 *  - Close unused pipe ends immediately after fork()
 *  - Re-build fd_set before every select() call
 *  - Set nfds = max_fd + 1 correctly
 *  - Non-blocking single-pass read after select() signals readiness
 *  - Graceful shutdown: waitpid() all children
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 select_example.cpp -o select_ipc
 *
 * Run:
 *   ./select_ipc
 */

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static constexpr int NUM_CHILDREN = 4;
static constexpr int READ_END     = 0;
static constexpr int WRITE_END    = 1;

/**
 * @brief Child process body: sleep a short time, write one message, exit.
 * @param child_id  Logical child index (0-based), used in the message.
 * @param write_fd  Write end of the pipe to the parent.
 */
static void run_child(int child_id, int write_fd) {
    // Each child sleeps a different amount so messages arrive out-of-order,
    // demonstrating that select() handles whichever arrives first.
    unsigned int delay_ms = static_cast<unsigned int>((NUM_CHILDREN - child_id) * 150);
    usleep(delay_ms * 1000u);

    char msg[128];
    int len = snprintf(msg, sizeof(msg),
                       "Hello from child %d (delay=%u ms)", child_id, delay_ms);
    if (len > 0) {
        if (write(write_fd, msg, static_cast<std::size_t>(len)) == -1) {
            // child write error is non-fatal for the demo; parent will get EOF
        }
    }
    close(write_fd);
}

int main() {
    // ── Create one pipe per child ─────────────────────────────────────────
    std::array<std::array<int, 2>, NUM_CHILDREN> pipes{};
    for (auto& p : pipes) {
        if (pipe(p.data()) == -1) {
            std::cerr << "[select] pipe() failed: " << std::strerror(errno) << '\n';
            return 1;
        }
    }

    // ── Fork children ─────────────────────────────────────────────────────
    std::array<pid_t, NUM_CHILDREN> pids{};
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        pids[i] = fork();
        if (pids[i] == -1) {
            std::cerr << "[select] fork() failed: " << std::strerror(errno) << '\n';
            return 1;
        }
        if (pids[i] == 0) {
            // Child: close all pipe ends except its own write end
            for (int j = 0; j < NUM_CHILDREN; ++j) {
                close(pipes[j][READ_END]);
                if (j != i) close(pipes[j][WRITE_END]);
            }
            run_child(i, pipes[i][WRITE_END]);
            return 0;
        }
        // Parent: close write ends it doesn't own
        close(pipes[i][WRITE_END]);
    }

    // ── Parent: use select() to read from whichever child writes first ────
    std::cout << "[select] Parent waiting for " << NUM_CHILDREN
              << " messages via select()...\n\n";

    int messages_received = 0;
    std::array<bool, NUM_CHILDREN> done{}; // which pipes have been closed

    while (messages_received < NUM_CHILDREN) {
        // Build the read set from still-open pipes
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = -1;

        for (int i = 0; i < NUM_CHILDREN; ++i) {
            if (!done[i]) {
                FD_SET(pipes[i][READ_END], &read_fds);
                if (pipes[i][READ_END] > max_fd) {
                    max_fd = pipes[i][READ_END];
                }
            }
        }

        if (max_fd == -1) break; // all pipes closed

        // Block until at least one FD is readable (no timeout)
        int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (ready == -1) {
            std::cerr << "[select] select() error: " << std::strerror(errno) << '\n';
            break;
        }

        // Inspect which FDs became readable
        for (int i = 0; i < NUM_CHILDREN; ++i) {
            if (done[i]) continue;
            if (!FD_ISSET(pipes[i][READ_END], &read_fds)) continue;

            std::array<char, 256> buf{};
            ssize_t n = read(pipes[i][READ_END], buf.data(), buf.size() - 1);
            if (n > 0) {
                buf[static_cast<std::size_t>(n)] = '\0';
                ++messages_received;
                std::cout << "[select] [msg " << messages_received << "/" << NUM_CHILDREN
                          << "] fd=" << pipes[i][READ_END]
                          << " | " << buf.data() << '\n';
            } else {
                // n==0: writer closed the pipe (EOF)
                close(pipes[i][READ_END]);
                done[i] = true;
            }
        }
    }

    // Close any remaining open read ends
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        if (!done[i]) close(pipes[i][READ_END]);
    }

    // Reap children
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        int status = 0;
        waitpid(pids[i], &status, 0);
    }

    std::cout << "\n[select] All " << NUM_CHILDREN << " messages received. Done.\n";
    return 0;
}
