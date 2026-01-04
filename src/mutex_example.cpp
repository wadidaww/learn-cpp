#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

std::mutex mtx; // Mutex for protecting shared data
int shared_variable = 0;

// Function to increment the shared variable
void increment() {
    for (int i = 0; i < 1000; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        shared_variable++;
    }
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.push_back(std::thread(increment));
    }

    for (auto& th : threads) {
        th.join();
    }

    std::cout << "Shared variable value: " << shared_variable << std::endl;

    return 0;
}
