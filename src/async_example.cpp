// STD::ASYNC
// ===========
// Main point: std::async launches a function asynchronously and returns std::future.
// Simpler than manual thread + promise for one-shot async computations.

#include <iostream>
#include <future>

int calculate_the_answer() { return 42; }

int main() {
    std::future<int> fut = std::async(calculate_the_answer);  // runs in background
    std::cout << "The answer is: " << fut.get() << "\n";     // blocks until done
}
