/**
 * @file epoll_example.cpp
 * @brief IPC multiplexing with epoll() — monitoring multiple pipe file
 *        descriptors from several child processes, using edge-triggered mode.
 *
 * ## epoll vs select/poll
 *
 * `select()` and `poll()` scan every registered descriptor on each call —
 * O(n) in the number of watched FDs.  On modern servers with tens of
 * thousands of connections, that becomes a bottleneck.
 *
 * `epoll` maintains a kernel-side event table.  The kernel marks only
 * *active* FDs and returns them directly — O(1) per ready event regardless
 * of how many FDs are registered.
 *
 * ## Key epoll syscalls
 *
 * ```c
 * // Create an epoll instance (returns a file descriptor itself)
 * int epoll_create1(int flags);  // flags=0 or EPOLL_CLOEXEC
 *
 * // Add/modify/remove a watched FD
 * int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
 *   // op: EPOLL_CTL_ADD | EPOLL_CTL_MOD | EPOLL_CTL_DEL
 *
 * // Wait for events (returns number of ready events)
 * int epoll_wait(int epfd, struct epoll_event *events,
 *                int maxevents, int timeout_ms); // -1 = block forever
 * ```
 *
 * ## Level-triggered (LT) vs Edge-triggered (ET)
 *
 *  - **Level-triggered** (default): epoll_wait() keeps returning a ready
 *    notification as long as data is available to read.  Simple to use but
 *    may incur extra syscalls.
 *
 *  - **Edge-triggered** (`EPOLLET`): epoll_wait() returns a notification
 *    *only once* when the FD transitions from not-ready to ready.  You MUST
 *    read/write in a loop until EAGAIN (or EWOULDBLOCK), otherwise you will
 *    miss data.  Requires the FD to be in non-blocking mode.
 *
 *    ET is generally used in high-performance servers because it minimises
 *    syscall overhead, but the application logic is more complex.
 *
 * ## This example
 *
 * 1. Parent creates NUM_CHILDREN pipe pairs.
 * 2. Read-ends are set to **non-blocking** mode (O_NONBLOCK).
 * 3. All read-ends are registered with epoll using EPOLLIN | EPOLLET.
 * 4. Each child sleeps a random-ish delay and writes one message.
 * 5. Parent calls epoll_wait() in a loop; for each ready FD it drains the
 *    pipe completely (required in ET mode — read until EAGAIN).
 * 6. Loop exits when all children have closed their write ends.
 *
 * Best practices demonstrated:
 *  - Use epoll_create1(EPOLL_CLOEXEC) to avoid FD leaks into child execs
 *  - Set O_NONBLOCK before registering with EPOLLET
 *  - Drain FD fully after EPOLLET notification (loop until EAGAIN)
 *  - Store per-FD context in epoll_event.data.ptr (cast from child index)
 *  - EPOLL_CTL_DEL before close() to avoid stale events
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 epoll_example.cpp -o epoll_ipc
 *
 * Run:
 *   ./epoll_ipc
 */

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static constexpr int NUM_CHILDREN  = 4;
static constexpr int MAX_EVENTS    = 16; // max events per epoll_wait() call
static constexpr int READ_END      = 0;
static constexpr int WRITE_END     = 1;

/**
 * @brief Set a file descriptor to non-blocking mode.
 * @return true on success, false on error.
 */
static bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

/**
 * @brief Child process body: sleep, write one message, exit.
 * @param child_id  Logical index used in the message text.
 * @param write_fd  Write end of the pipe to the parent.
 */
static void run_child(int child_id, int write_fd) {
    unsigned int delay_ms = static_cast<unsigned int>((NUM_CHILDREN - child_id) * 200);
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
            std::cerr << "[epoll] pipe() failed: " << std::strerror(errno) << '\n';
            return 1;
        }
    }

    // ── Create epoll instance ─────────────────────────────────────────────
    // EPOLL_CLOEXEC: automatically close the epoll FD in child execs
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        std::cerr << "[epoll] epoll_create1() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    // ── Register pipe read-ends with epoll (before forking) ──────────────
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        // Edge-triggered mode requires non-blocking I/O
        if (!set_nonblocking(pipes[i][READ_END])) {
            std::cerr << "[epoll] set_nonblocking() failed for pipe " << i
                      << ": " << std::strerror(errno) << '\n';
            close(epoll_fd);
            return 1;
        }

        struct epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET; // read-ready, edge-triggered
        // Store the child index as user data so we can identify the pipe later
        ev.data.u32 = static_cast<uint32_t>(i);

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipes[i][READ_END], &ev) == -1) {
            std::cerr << "[epoll] epoll_ctl(ADD) failed for pipe " << i
                      << ": " << std::strerror(errno) << '\n';
            close(epoll_fd);
            return 1;
        }
    }

    std::cout << "[epoll] Registered " << NUM_CHILDREN
              << " pipe read-ends with epoll (EPOLLIN|EPOLLET).\n\n";

    // ── Fork children ─────────────────────────────────────────────────────
    std::array<pid_t, NUM_CHILDREN> pids{};
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        pids[i] = fork();
        if (pids[i] == -1) {
            std::cerr << "[epoll] fork() failed: " << std::strerror(errno) << '\n';
            close(epoll_fd);
            return 1;
        }
        if (pids[i] == 0) {
            // Child: close epoll FD and all pipe ends except its own write end
            close(epoll_fd);
            for (int j = 0; j < NUM_CHILDREN; ++j) {
                close(pipes[j][READ_END]);
                if (j != i) close(pipes[j][WRITE_END]);
            }
            run_child(i, pipes[i][WRITE_END]);
            return 0;
        }
        // Parent: close write end (child owns it)
        close(pipes[i][WRITE_END]);
    }

    // ── Parent: event loop ────────────────────────────────────────────────
    std::cout << "[epoll] Parent event loop started. Waiting for children...\n\n";

    int pipes_open       = NUM_CHILDREN; // how many read-ends still active
    int messages_received = 0;
    std::array<struct epoll_event, MAX_EVENTS> events{};

    while (pipes_open > 0) {
        int ready = epoll_wait(epoll_fd, events.data(),
                               static_cast<int>(events.size()), -1 /*block*/);
        if (ready == -1) {
            if (errno == EINTR) continue; // interrupted by signal, retry
            std::cerr << "[epoll] epoll_wait() error: " << std::strerror(errno) << '\n';
            break;
        }

        for (int e = 0; e < ready; ++e) {
            int child_idx = static_cast<int>(events[e].data.u32);
            int read_fd   = pipes[child_idx][READ_END];

            if (events[e].events & EPOLLIN) {
                // ── ET mode: drain ALL available data before returning ────
                // With EPOLLET, we only get one notification per write burst.
                // We must read in a loop until EAGAIN, otherwise remaining
                // bytes will never trigger another event.
                std::array<char, 256> buf{};
                ssize_t n = 0;

                while (true) {
                    n = read(read_fd, buf.data(), buf.size() - 1);
                    if (n > 0) {
                        buf[static_cast<std::size_t>(n)] = '\0';
                        ++messages_received;
                        std::cout << "[epoll] [event " << messages_received
                                  << "] fd=" << read_fd
                                  << " child=" << child_idx
                                  << " | " << buf.data() << '\n';
                    } else if (n == 0) {
                        // EOF: writer closed pipe
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, read_fd, nullptr);
                        close(read_fd);
                        --pipes_open;
                        break;
                    } else {
                        // n == -1
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // No more data available right now — ET done
                            break;
                        }
                        std::cerr << "[epoll] read() error: "
                                  << std::strerror(errno) << '\n';
                        break;
                    }
                }
            } else if (events[e].events & (EPOLLERR | EPOLLHUP)) {
                // EPOLLIN was not set — no data arrived, just a hangup/error.
                // (When EPOLLIN and EPOLLHUP arrive together, the branch above
                //  handles EOF via read() returning 0.)
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, read_fd, nullptr);
                close(read_fd);
                --pipes_open;
            }
        }
    }

    close(epoll_fd);

    // Reap children
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        int status = 0;
        waitpid(pids[i], &status, 0);
    }

    std::cout << "\n[epoll] All " << NUM_CHILDREN << " messages received. Done.\n";
    return 0;
}
