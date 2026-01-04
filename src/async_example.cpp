#include <iostream>
#include <future>

int calculate_the_answer() {
    // Perform some work
    return 42;
}

int main() {
    // Run calculate_the_answer asynchronously
    std::future<int> fut = std::async(calculate_the_answer);

    // a non-blocking wait for the result
    std::cout << "Waiting for the result..." << std::endl;
    int result = fut.get();

    std::cout << "The answer is: " << result << std::endl;

    return 0;
}
