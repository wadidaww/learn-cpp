// lockfree_stack_example.cpp
#include <atomic>
#include <cstdint>
#include <cassert>
#include <memory>
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>

// ---------- LockFreeStack (Treiber) ----------
// Notes:
// - Uses packed (pointer | tag) in an uintptr_t atomic to mitigate ABA.
// - Requires nodes to be sufficiently aligned so low bits of pointer are zero.
// - Does NOT implement a full safe memory-reclamation scheme (hazard pointers/epochs).
//   Deleting nodes after successful pop here is simple but may be unsafe in presence
//   of concurrent readers unless you use hazard pointers or another reclamation scheme.

template<typename T>
class LockFreeStack {
private:
    struct Node {
        T value;
        Node* next;
        Node(const T& v) : value(v), next(nullptr) {}
    };

    // We pack pointer and tag into uintptr_t:
    // [ pointer (high bits) | tag (low bits) ]
    // Assume at least 4 bytes alignment => low 2 bits free, but we try to use 16 alignment
    static constexpr uintptr_t TAG_BITS = 16; // number of low bits used for tag
    static_assert(TAG_BITS < (sizeof(uintptr_t) * 8 - 3), "TAG_BITS too large");

    static constexpr uintptr_t TAG_MASK = ( (uintptr_t(1) << TAG_BITS) - 1 );

    std::atomic<uintptr_t> head_; // packed pointer + tag

    static uintptr_t pack(Node* p, uintptr_t tag) {
        uintptr_t ptr = reinterpret_cast<uintptr_t>(p);
        assert((ptr & TAG_MASK) == 0 && "Node pointer not aligned enough for tagging");
        return (ptr | (tag & TAG_MASK));
    }

    static void unpack(uintptr_t packed, Node*& p_out, uintptr_t& tag_out) {
        p_out = reinterpret_cast<Node*>(packed & ~TAG_MASK);
        tag_out = (packed & TAG_MASK);
    }

public:
    LockFreeStack() noexcept : head_(pack(nullptr, 0)) {}

    ~LockFreeStack() {
        // Drain and free remaining nodes (single-threaded destructor)
        Node* n;
        uintptr_t t;
        unpack(head_.load(std::memory_order_relaxed), n, t);
        while (n) {
            Node* nxt = n->next;
            delete n;
            n = nxt;
        }
    }

    void push(const T& value) {
        Node* new_node = new Node(value);
        // ensure alignment: request aligned allocation if needed
        // (default new on typical platforms yields sufficient alignment for pointers)
        for (;;) {
            uintptr_t old = head_.load(std::memory_order_acquire);
            Node* old_ptr;
            uintptr_t old_tag;
            unpack(old, old_ptr, old_tag);

            new_node->next = old_ptr;
            uintptr_t new_tag = (old_tag + 1) & TAG_MASK;
            uintptr_t desired = pack(new_node, new_tag);
            if (head_.compare_exchange_weak(old, desired,
                                            std::memory_order_release,
                                            std::memory_order_acquire)) {
                return; // pushed successfully
            }
            // CAS failed — retry
        }
    }

    // returns true and sets out if pop succeeded; false if stack empty
    bool pop(T& out) {
        for (;;) {
            uintptr_t old = head_.load(std::memory_order_acquire);
            Node* old_ptr;
            uintptr_t old_tag;
            unpack(old, old_ptr, old_tag);
            if (!old_ptr) {
                return false; // empty
            }
            Node* next = old_ptr->next;
            uintptr_t new_tag = (old_tag + 1) & TAG_MASK;
            uintptr_t desired = pack(next, new_tag);
            if (head_.compare_exchange_weak(old, desired,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
                // Successfully popped old_ptr.
                out = std::move(old_ptr->value);
                delete old_ptr; // **CAUTION**: unsafe reclamation without hazard pointers!
                return true;
            }
            // CAS failed — retry
        }
    }

    bool empty() const noexcept {
        uintptr_t packed = head_.load(std::memory_order_acquire);
        Node* p;
        uintptr_t tag;
        unpack(packed, p, tag);
        return p == nullptr;
    }
};

int main() {
    LockFreeStack<int> s;

    const int NUM_PUSHERS = 4;
    const int NUM_POPPERS = 4;
    const int PER_THREAD = 50'000;

    std::atomic<int> push_sum{0};
    std::atomic<int> pop_sum{0};

    // pushers
    std::vector<std::thread> threads;
    for (int p = 0; p < NUM_PUSHERS; ++p) {
        threads.emplace_back([&s, &push_sum, p]() {
            int local_sum = 0;
            for (int i = 1; i <= PER_THREAD; ++i) {
                int val = p * PER_THREAD + i;
                s.push(val);
                local_sum += val;
            }
            push_sum.fetch_add(local_sum, std::memory_order_relaxed);
        });
    }

    // poppers
    for (int c = 0; c < NUM_POPPERS; ++c) {
        threads.emplace_back([&s, &pop_sum]() {
            int local_sum = 0;
            int val;
            // try to pop until we think producers are done (simple heuristic)
            // For demonstration we attempt many pops. A real test would synchronize threads.
            for (int i = 0; i < 200000; ++i) {
                if (s.pop(val)) local_sum += val;
            }
            pop_sum.fetch_add(local_sum, std::memory_order_relaxed);
        });
    }

    for (auto& t : threads) t.join();

    std::cout << "Pushed total = " << push_sum.load() << "\n";
    std::cout << "Popped total = " << pop_sum.load() << "\n";
    std::cout << "Remaining empty? " << std::boolalpha << s.empty() << "\n";

    return 0;
}
