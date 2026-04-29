/**
 * @file receiver.cpp
 * @brief POSIX message-queue consumer: opens the queue and receives messages
 *        in priority order (highest first).
 *
 * Notice that messages are *not* received in the order they were sent –
 * they are dequeued by descending priority, which is one of the key
 * advantages of POSIX message queues over plain pipes.
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 receiver.cpp -lrt -o mq_receiver
 *
 * Usage:
 *   Terminal 1: ./mq_receiver   (start first – it creates the queue)
 *   Terminal 2: ./mq_sender
 */

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <mqueue.h>
#include <sys/stat.h>
#include <unistd.h>

static constexpr const char* QUEUE_NAME  = "/ipc_demo_mqueue";
static constexpr long        MAX_MSGS    = 10;
static constexpr long        MAX_MSG_SZ  = 256;
static constexpr int         TOTAL_MSGS  = 5;

int main() {
    struct mq_attr attr{};
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = MAX_MSGS;
    attr.mq_msgsize = MAX_MSG_SZ;
    attr.mq_curmsgs = 0;

    // Create queue (or open existing) for reading
    mqd_t mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0600, &attr);
    if (mq == static_cast<mqd_t>(-1)) {
        std::cerr << "[mq_receiver] mq_open() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    std::cout << "[mq_receiver] Queue ready. Waiting for sender to enqueue all messages...\n";

    // Brief pause to let sender enqueue all messages at once so we can
    // demonstrate that mq_receive() dequeues in descending priority order
    // regardless of the order in which messages were sent.
    usleep(800'000); // 800 ms

    std::cout << "[mq_receiver] Reading " << TOTAL_MSGS << " messages"
              << " (received in PRIORITY order, not send order):\n\n";

    std::array<char, MAX_MSG_SZ + 1> buf{};

    for (int i = 0; i < TOTAL_MSGS; ++i) {
        unsigned int prio = 0;
        ssize_t bytes = mq_receive(mq, buf.data(), buf.size() - 1, &prio);
        if (bytes == -1) {
            std::cerr << "[mq_receiver] mq_receive() failed: " << std::strerror(errno) << '\n';
            mq_close(mq);
            return 1;
        }
        buf[static_cast<std::size_t>(bytes)] = '\0';
        std::cout << "[mq_receiver] [prio=" << prio << "] " << buf.data() << '\n';
    }

    mq_close(mq);
    std::cout << "\n[mq_receiver] All messages received. Done.\n";
    return 0;
}
