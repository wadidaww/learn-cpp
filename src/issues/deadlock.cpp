#include <iostream>
#include <thread>
#include <mutex>
#include <chrono> // For std::this_thread::sleep_for

std::mutex mutex1;
std::mutex mutex2;

void threadFunction1() {
    std::cout << "Thread 1: Attempting to lock mutex1..." << std::endl;
    std::lock_guard<std::mutex> lock1(mutex1); // Locks mutex1
    std::cout << "Thread 1: Locked mutex1." << std::endl;

    // Introduce a small delay to increase the chance of deadlock
    std::this_thread::sleep_for(std::chrono::milliseconds(0));

    std::cout << "Thread 1: Attempting to lock mutex2..." << std::endl;
    std::lock_guard<std::mutex> lock2(mutex2); // Attempts to lock mutex2
    std::cout << "Thread 1: Locked mutex2." << std::endl;

    std::cout << "Thread 1: Both mutexes locked. Performing work..." << std::endl;
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "Thread 1: Work complete. Releasing mutexes." << std::endl;
}

void threadFunction2() {
    std::cout << "Thread 2: Attempting to lock mutex2..." << std::endl;
    std::lock_guard<std::mutex> lock2(mutex2); // Locks mutex2
    std::cout << "Thread 2: Locked mutex2." << std::endl;

    // Introduce a small delay to increase the chance of deadlock
    std::this_thread::sleep_for(std::chrono::milliseconds(0));

    std::cout << "Thread 2: Attempting to lock mutex1..." << std::endl;
    std::lock_guard<std::mutex> lock1(mutex1); // Attempts to lock mutex1
    std::cout << "Thread 2: Locked mutex1." << std::endl;

    std::cout << "Thread 2: Both mutexes locked. Performing work..." << std::endl;
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "Thread 2: Work complete. Releasing mutexes." << std::endl;
}

int main() {
    std::thread t1(threadFunction1);
    std::thread t2(threadFunction2);

    t1.join();
    t2.join();

    std::cout << "Main: All threads finished." << std::endl;
    return 0;
}