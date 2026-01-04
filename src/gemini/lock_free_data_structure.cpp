#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

template <typename T>
class LockFreeQueue {
private:
    struct Node {
        T value;
        Node* next;

        Node(const T& val) : value(val), next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    LockFreeQueue() {
        Node* dummy = new Node(T()); // Create a dummy node
        head.store(dummy);
        tail.store(dummy);
    }

    ~LockFreeQueue() {
        while (Node* oldHead = head.load()) {
            head.store(oldHead->next);
            delete oldHead;
        }
    }

    // Producer (enqueue) function
    void push(const T& value) {
        Node* newNode = new Node(value);
        Node* oldTail = tail.load();
        
        while (!tail.compare_exchange_weak(oldTail, newNode)) {
            // Keep trying to update the tail if it was modified by another thread.
            // In a single-producer scenario, this loop is not strictly needed
            // but is good practice for general lock-free algorithms.
        }
        oldTail->next = newNode;
    }

    // Consumer (dequeue) function
    bool pop(T& value) {
        Node* oldHead = head.load();
        
        // Check if the queue is empty.
        if (oldHead == tail.load()) {
            return false;
        }

        Node* newHead = oldHead->next;
        if (newHead) {
            value = newHead->value;
            head.store(newHead);
            delete oldHead; // Clean up the old dummy node
            return true;
        }
        return false;
    }
};

void producer(LockFreeQueue<int>& q) {
    for (int i = 0; i < 1000; ++i) {
        q.push(i);
    }
}

void consumer(LockFreeQueue<int>& q, std::vector<int>& results) {
    int value;
    while (results.size() < 1000) {
        if (q.pop(value)) {
            results.push_back(value);
        }
    }
}

int main() {
    LockFreeQueue<int> q;
    std::vector<int> results;

    std::thread producer_thread(producer, std::ref(q));
    std::thread consumer_thread(consumer, std::ref(q), std::ref(results));

    producer_thread.join();
    consumer_thread.join();

    std::cout << "Successfully processed " << results.size() << " items." << std::endl;
    // Note: The order of items in 'results' is not guaranteed
    // due to the non-blocking nature of the consumer.

    return 0;
}