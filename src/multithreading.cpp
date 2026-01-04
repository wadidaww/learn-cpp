#include <iostream>
#include <thread>

// Function to be executed by the thread
void thread_function() {
    std::cout << "Hello from thread!" << std::endl;
}

int main() {
    // Create a new thread and execute thread_function
    std::thread t(thread_function);

    // Wait for the thread to finish
    t.join();

    std::cout << "Hello from main!" << std::endl;

    return 0;
}
