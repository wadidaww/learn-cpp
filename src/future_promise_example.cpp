#include <iostream>
#include <thread>
#include <future>

void calculate_the_answer(std::promise<int> prom) {
    // Perform some work
    int result = 42;
    prom.set_value(result);
}

int main() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();

    std::thread t(calculate_the_answer, std::move(prom));

    // a non-blocking wait for the result
    std::cout << "Waiting for the result..." << std::endl;
    int result = fut.get();

    std::cout << "The answer is: " << result << std::endl;

    t.join();

    return 0;
}
