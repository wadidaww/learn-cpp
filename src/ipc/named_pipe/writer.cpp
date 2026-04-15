/**
 * @file writer.cpp
 * @brief Named-pipe (FIFO) producer: creates the FIFO and writes messages to it.
 *
 * Named pipes (FIFOs) extend anonymous pipes by giving them a filesystem path,
 * so any two *unrelated* processes can communicate through them.
 *
 * Key characteristics:
 *  - Persistent filesystem entry (until unlink()'d)
 *  - Unidirectional byte stream (same as anonymous pipes)
 *  - Opening blocks until *both* ends are open (open() is a rendezvous point)
 *  - Kernel-buffered – same semantics as anonymous pipes
 *
 * Best practices demonstrated:
 *  - Use O_WRONLY on the write side (no accidental reads)
 *  - Unlink the FIFO only from the creator to avoid TOCTOU races
 *  - Check every syscall return value
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 writer.cpp -o fifo_writer
 *
 * Usage:
 *   Terminal 1: ./fifo_reader   (start the reader first)
 *   Terminal 2: ./fifo_writer
 */

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static constexpr const char* FIFO_PATH = "/tmp/ipc_demo_fifo";

int main() {
    // Create the FIFO if it doesn't exist yet
    // mkfifo() fails with EEXIST if already present – that's fine
    if (mkfifo(FIFO_PATH, 0600) == -1 && errno != EEXIST) {
        std::cerr << "[writer] mkfifo() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    std::cout << "[writer] FIFO created at " << FIFO_PATH
              << ". Waiting for reader to connect...\n";

    // open() blocks here until the reader opens the read end
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        std::cerr << "[writer] open() failed: " << std::strerror(errno) << '\n';
        unlink(FIFO_PATH);
        return 1;
    }

    std::cout << "[writer] Reader connected. Sending messages...\n";

    const std::string messages[] = {
        "FIFO message 1: Hello!\n",
        "FIFO message 2: Named pipes are filesystem-level IPC.\n",
        "FIFO message 3: Goodbye!\n"
    };

    for (const auto& msg : messages) {
        ssize_t written = write(fd, msg.c_str(), msg.size());
        if (written == -1) {
            std::cerr << "[writer] write() error: " << std::strerror(errno) << '\n';
            break;
        }
        std::cout << "[writer] Sent: " << msg;
        usleep(200'000); // 200 ms between messages
    }

    close(fd);
    unlink(FIFO_PATH); // Clean up the filesystem entry
    std::cout << "[writer] Done.\n";
    return 0;
}
