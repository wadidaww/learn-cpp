/**
 * @file sender.cpp
 * @brief POSIX message-queue producer: opens a queue and sends structured
 *        messages with different priorities.
 *
 * POSIX message queues (mqueue) differ from pipes in two important ways:
 *  1. **Message boundaries are preserved** – each send/receive is an atomic
 *     unit (unlike the byte-stream of pipes).
 *  2. **Priority** – messages are dequeued in descending priority order,
 *     making this mechanism ideal for event/notification systems.
 *
 * Key characteristics:
 *  - Kernel-buffered with configurable depth and message size
 *  - Named via the filesystem (like FIFOs): visible under /dev/mqueue
 *  - Supports asynchronous notification via mq_notify()
 *  - Persists until mq_unlink() is called or the system reboots
 *
 * Best practices demonstrated:
 *  - Set explicit attr (maxmsg, msgsize) rather than relying on defaults
 *  - Use priorities to distinguish high- vs low-importance events
 *  - Always unlink the queue from one side to avoid leaks
 *
 * Build (standalone):
 *   g++ -std=c++17 -Wall -Wextra -O2 sender.cpp -lrt -o mq_sender
 *
 * Usage:
 *   Terminal 1: ./mq_receiver
 *   Terminal 2: ./mq_sender
 */

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <mqueue.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static constexpr const char* QUEUE_NAME  = "/ipc_demo_mqueue";
static constexpr long        MAX_MSGS    = 10;
static constexpr long        MAX_MSG_SZ  = 256;

// Message priorities (higher = delivered first)
static constexpr unsigned int PRIO_HIGH   = 5;
static constexpr unsigned int PRIO_NORMAL = 3;
static constexpr unsigned int PRIO_LOW    = 1;

int main() {
    struct mq_attr attr{};
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = MAX_MSGS;
    attr.mq_msgsize = MAX_MSG_SZ;
    attr.mq_curmsgs = 0;

    // O_CREAT | O_WRONLY: create if needed, open write-only
    mqd_t mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0600, &attr);
    if (mq == static_cast<mqd_t>(-1)) {
        std::cerr << "[mq_sender] mq_open() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    // Define messages with their priorities
    struct Message {
        std::string      text;
        unsigned int     priority;
    };

    const Message messages[] = {
        {"LOW PRIORITY: routine status update",        PRIO_LOW   },
        {"HIGH PRIORITY: critical error detected!",    PRIO_HIGH  },
        {"NORMAL PRIORITY: processing step complete",  PRIO_NORMAL},
        {"HIGH PRIORITY: urgent shutdown request",     PRIO_HIGH  },
        {"LOW PRIORITY: background housekeeping done", PRIO_LOW   }
    };

    std::cout << "[mq_sender] Enqueuing " << std::size(messages) << " messages rapidly"
              << " so the receiver sees them all at once and can apply priority ordering:\n";

    for (const auto& m : messages) {
        std::cout << "  [prio=" << m.priority << "] " << m.text << '\n';
        if (mq_send(mq, m.text.c_str(), m.text.size(), m.priority) == -1) {
            std::cerr << "[mq_sender] mq_send() failed: " << std::strerror(errno) << '\n';
            mq_close(mq);
            mq_unlink(QUEUE_NAME);
            return 1;
        }
    }
    std::cout << "[mq_sender] All messages enqueued.\n";

    mq_close(mq);
    mq_unlink(QUEUE_NAME);
    std::cout << "[mq_sender] All messages sent. Queue unlinked.\n";
    return 0;
}
