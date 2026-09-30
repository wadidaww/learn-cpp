// PROMISE & FUTURE
// =================
// Main point: std::promise sends a value, std::future receives it.
// This is how you pass data from a worker thread back to the main thread.

#include <iostream>
#include <thread>
#include <future>

void calculate_the_answer(std::promise<int> prom) {
    prom.set_value(42);  // set the result (only once)
}

int main() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();
    std::thread t(calculate_the_answer, std::move(prom));
    std::cout << "The answer is: " << fut.get() << "\n";  // blocks until value is set
    t.join();
}
