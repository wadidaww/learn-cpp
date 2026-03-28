/**
 * @file reader.cpp
 * @brief Named-pipe (FIFO) consumer: opens the FIFO and reads messages from it.
 *
 * Run this program *before* the writer so that the writer's open() does not
 * block indefinitely waiting for the read end.
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 reader.cpp -o fifo_reader
 *
 * Usage:
 *   Terminal 1: ./fifo_reader   (start first)
 *   Terminal 2: ./fifo_writer
 */

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static constexpr const char* FIFO_PATH = "/tmp/ipc_demo_fifo";

int main() {
    // Create the FIFO if writer hasn't done so yet
    if (mkfifo(FIFO_PATH, 0600) == -1 && errno != EEXIST) {
        std::cerr << "[reader] mkfifo() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    std::cout << "[reader] Opening FIFO for reading (will block until writer connects)...\n";

    // open() blocks here until the write end is opened by the writer
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        std::cerr << "[reader] open() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    std::cout << "[reader] Writer connected. Reading messages...\n";

    std::array<char, 512> buf{};
    ssize_t bytes_read = 0;

    while ((bytes_read = read(fd, buf.data(), buf.size() - 1)) > 0) {
        buf[static_cast<std::size_t>(bytes_read)] = '\0';
        std::cout << "[reader] Received: " << buf.data();
    }

    if (bytes_read == -1) {
        std::cerr << "[reader] read() error: " << std::strerror(errno) << '\n';
    }

    close(fd);
    std::cout << "[reader] EOF – writer closed. Done.\n";
    return 0;
}
