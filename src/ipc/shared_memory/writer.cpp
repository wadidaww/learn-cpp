/**
 * @file writer.cpp
 * @brief Shared-memory producer: writes a counter into shared memory and
 *        signals the reader via a POSIX semaphore.
 *
 * Shared memory is the *fastest* IPC mechanism because data is exchanged via
 * memory directly mapped into both processes' address spaces – no kernel
 * copy is performed (unlike pipes or message queues).
 *
 * The tradeoff is that shared memory provides *no synchronisation* on its
 * own. This example uses a pair of POSIX named semaphores:
 *
 *   SEM_EMPTY – counts how many "slots" are free to be written (starts at 1)
 *   SEM_FULL  – counts how many "slots" contain new data (starts at 0)
 *
 * This is the classic producer/consumer semaphore protocol, which gives:
 *  - No busy-waiting (sem_wait blocks when no work is available)
 *  - No data races (only one side accesses shared memory at a time)
 *  - No missed wakeups
 *
 * Key characteristics of POSIX shared memory:
 *  - Created with shm_open() + ftruncate() + mmap()
 *  - Zero-copy between processes
 *  - Persists until shm_unlink() is called or the system reboots
 *
 * Best practices demonstrated:
 *  - RAII-style cleanup in error paths
 *  - Explicit memory barrier via atomic/semaphore before reading/writing
 *  - Unlink shared memory and semaphores from the creator only
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 writer.cpp -lrt -lpthread -o shm_writer
 *
 * Usage:
 *   Terminal 1: ./shm_reader
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

// ── Shared names ────────────────────────────────────────────────────────────
static constexpr const char* SHM_NAME   = "/ipc_demo_shm";
static constexpr const char* SEM_EMPTY  = "/ipc_demo_sem_empty";
static constexpr const char* SEM_FULL   = "/ipc_demo_sem_full";
static constexpr int         MSG_COUNT  = 5;

// ── Shared data layout ──────────────────────────────────────────────────────
struct SharedData {
    int  value;
    char message[64];
};

int main() {
    // ── Create shared memory ─────────────────────────────────────────────
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (shm_fd == -1) {
        std::cerr << "[shm_writer] shm_open() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    if (ftruncate(shm_fd, static_cast<off_t>(sizeof(SharedData))) == -1) {
        std::cerr << "[shm_writer] ftruncate() failed: " << std::strerror(errno) << '\n';
        shm_unlink(SHM_NAME);
        return 1;
    }

    auto* data = static_cast<SharedData*>(
        mmap(nullptr, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    close(shm_fd); // fd no longer needed after mmap

    if (data == MAP_FAILED) {
        std::cerr << "[shm_writer] mmap() failed: " << std::strerror(errno) << '\n';
        shm_unlink(SHM_NAME);
        return 1;
    }

    // ── Create semaphores ────────────────────────────────────────────────
    // SEM_EMPTY starts at 1: one free slot available
    sem_t* sem_empty = sem_open(SEM_EMPTY, O_CREAT, 0600, 1);
    // SEM_FULL starts at 0: no data available yet
    sem_t* sem_full  = sem_open(SEM_FULL,  O_CREAT, 0600, 0);

    if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED) {
        std::cerr << "[shm_writer] sem_open() failed: " << std::strerror(errno) << '\n';
        munmap(data, sizeof(SharedData));
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_EMPTY);
        sem_unlink(SEM_FULL);
        return 1;
    }

    std::cout << "[shm_writer] Shared memory and semaphores ready. Writing " << MSG_COUNT << " values...\n";

    for (int i = 1; i <= MSG_COUNT; ++i) {
        // Wait for a free slot
        sem_wait(sem_empty);

        // ── Critical section: write shared data ─────────────────────────
        data->value = i;
        snprintf(data->message, sizeof(data->message), "Hello from writer, value=%d", i);
        // ────────────────────────────────────────────────────────────────

        std::cout << "[shm_writer] Wrote: value=" << data->value
                  << "  msg=\"" << data->message << "\"\n";

        // Signal that new data is ready
        sem_post(sem_full);

        usleep(300'000); // 300 ms – simulates work between writes
    }

    // ── Cleanup ──────────────────────────────────────────────────────────
    sem_close(sem_empty);
    sem_close(sem_full);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    munmap(data, sizeof(SharedData));
    shm_unlink(SHM_NAME);

    std::cout << "[shm_writer] Done.\n";
    return 0;
}
