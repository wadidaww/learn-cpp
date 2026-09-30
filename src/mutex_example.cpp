// MUTEX & LOCK_GUARD
// ===================
// Main point: std::mutex + std::lock_guard prevents data races on shared data.
// lock_guard is RAII — automatically unlocks when it goes out of scope.

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

std::mutex mtx;
int shared_variable = 0;

void increment() {
    for (int i = 0; i < 1000; ++i) {
        std::lock_guard<std::mutex> lock(mtx);  // locked here, unlocked when lock goes out of scope
        shared_variable++;
    }
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) threads.push_back(std::thread(increment));
    for (auto& th : threads) th.join();
    std::cout << "Shared variable: " << shared_variable << " (expected 10000)\n";
}
