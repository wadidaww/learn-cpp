/**
 * @file pipe_example.cpp
 * @brief Demonstrates anonymous pipes for IPC between a parent and child process.
 *
 * Anonymous pipes are the simplest IPC mechanism. They provide a unidirectional
 * byte stream between two related processes (parent ↔ child). Data written to
 * the write-end of the pipe can be read from the read-end.
 *
 * Key characteristics:
 *  - Unidirectional (use two pipes for bidirectional communication)
 *  - Only usable between processes that share the pipe file descriptors
 *    (i.e. parent/child or sibling processes after a fork)
 *  - Kernel-buffered (~64 KB on Linux) – write() blocks when the buffer is full
 *  - No persistence: data exists only while at least one end is open
 *
 * Best practices demonstrated here:
 *  - Close unused pipe ends immediately after fork() to avoid descriptor leaks
 *    and to ensure the reader gets EOF when the writer exits
 *  - Check every syscall return value
 *  - Use RAII-style cleanup (close before exit)
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 pipe_example.cpp -o pipe_example
 *
 * Run:
 *   ./pipe_example
 */

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Pipe file descriptor indices
static constexpr int READ_END  = 0;
static constexpr int WRITE_END = 1;

/**
 * @brief Child process: reads messages from the pipe until EOF and prints them.
 * @param read_fd  The read end of the pipe.
 */
static void child_reader(int read_fd) {
    std::array<char, 256> buf{};
    ssize_t bytes_read = 0;

    std::cout << "[child] Waiting for messages...\n";

    while ((bytes_read = read(read_fd, buf.data(), buf.size() - 1)) > 0) {
        buf[static_cast<std::size_t>(bytes_read)] = '\0';
        std::cout << "[child] Received: " << buf.data() << '\n';
    }

    if (bytes_read == -1) {
        std::cerr << "[child] read() error: " << std::strerror(errno) << '\n';
    }

    std::cout << "[child] Pipe closed by parent – exiting.\n";
    close(read_fd);
}

/**
 * @brief Parent process: sends a series of messages through the pipe.
 * @param write_fd  The write end of the pipe.
 */
static void parent_writer(int write_fd) {
    const std::string messages[] = {
        "Hello from parent (msg 1)",
        "Inter-Process Communication via pipes (msg 2)",
        "This is the last message (msg 3)"
    };

    for (const auto& msg : messages) {
        std::cout << "[parent] Sending: " << msg << '\n';
        ssize_t written = write(write_fd, msg.c_str(), msg.size());
        if (written == -1) {
            std::cerr << "[parent] write() error: " << std::strerror(errno) << '\n';
            break;
        }
        // Small delay so output is easier to follow in the terminal
        usleep(100'000); // 100 ms
    }

    // Closing the write end signals EOF to the reader
    close(write_fd);
    std::cout << "[parent] Write end closed.\n";
}

int main() {
    std::array<int, 2> pipe_fds{};

    if (pipe(pipe_fds.data()) == -1) {
        std::cerr << "pipe() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "fork() failed: " << std::strerror(errno) << '\n';
        close(pipe_fds[READ_END]);
        close(pipe_fds[WRITE_END]);
        return 1;
    }

    if (pid == 0) {
        // ── Child ──────────────────────────────────────────────────────────
        // Close the write end – child only reads
        close(pipe_fds[WRITE_END]);
        child_reader(pipe_fds[READ_END]);
        return 0;
    }

    // ── Parent ─────────────────────────────────────────────────────────────
    // Close the read end – parent only writes
    close(pipe_fds[READ_END]);
    parent_writer(pipe_fds[WRITE_END]);

    // Wait for child to finish
    int status = 0;
    waitpid(pid, &status, 0);
    std::cout << "[parent] Child exited with status "
              << WEXITSTATUS(status) << ".\n";

    return 0;
}
