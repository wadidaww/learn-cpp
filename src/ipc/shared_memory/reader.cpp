/**
 * @file reader.cpp
 * @brief Shared-memory consumer: reads counter values from shared memory
 *        synchronised with POSIX semaphores.
 *
 * See writer.cpp for a full explanation of the shared-memory + semaphore design.
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 reader.cpp -lrt -lpthread -o shm_reader
 *
 * Usage:
 *   Terminal 1: ./shm_reader   (start first)
 *   Terminal 2: ./shm_writer
 */

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static constexpr const char* SHM_NAME  = "/ipc_demo_shm";
static constexpr const char* SEM_EMPTY = "/ipc_demo_sem_empty";
static constexpr const char* SEM_FULL  = "/ipc_demo_sem_full";
static constexpr int         MSG_COUNT = 5;

struct SharedData {
    int  value;
    char message[64];
};

int main() {
    // ── Open shared memory (created by writer) ───────────────────────────
    // Poll until the writer has created the shared-memory object
    int shm_fd = -1;
    for (int retry = 0; retry < 20 && shm_fd == -1; ++retry) {
        shm_fd = shm_open(SHM_NAME, O_RDONLY, 0);
        if (shm_fd == -1) {
            usleep(100'000); // wait 100 ms before retrying
        }
    }
    if (shm_fd == -1) {
        std::cerr << "[shm_reader] shm_open() failed after retries: "
                  << std::strerror(errno) << '\n';
        return 1;
    }

    auto* data = static_cast<const SharedData*>(
        mmap(nullptr, sizeof(SharedData), PROT_READ, MAP_SHARED, shm_fd, 0));
    close(shm_fd);

    if (data == MAP_FAILED) {
        std::cerr << "[shm_reader] mmap() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    // ── Open semaphores (created by writer) ─────────────────────────────
    sem_t* sem_empty = sem_open(SEM_EMPTY, 0);
    sem_t* sem_full  = sem_open(SEM_FULL,  0);

    if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED) {
        std::cerr << "[shm_reader] sem_open() failed: " << std::strerror(errno) << '\n';
        munmap(const_cast<SharedData*>(data), sizeof(SharedData));
        return 1;
    }

    std::cout << "[shm_reader] Connected. Waiting for " << MSG_COUNT << " values...\n";

    for (int i = 0; i < MSG_COUNT; ++i) {
        // Block until writer signals new data
        sem_wait(sem_full);

        // ── Critical section: read shared data ──────────────────────────
        std::cout << "[shm_reader] Read: value=" << data->value
                  << "  msg=\"" << data->message << "\"\n";
        // ────────────────────────────────────────────────────────────────

        // Signal that the slot is free again
        sem_post(sem_empty);
    }

    sem_close(sem_empty);
    sem_close(sem_full);
    munmap(const_cast<SharedData*>(data), sizeof(SharedData));

    std::cout << "[shm_reader] Done.\n";
    return 0;
}
