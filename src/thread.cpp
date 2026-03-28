// #include <chrono>
// #include <iostream>
// #include <mutex>
// #include <thread>
 
// int g_num = 0; // protected by g_num_mutex
// std::mutex g_num_mutex;
 
// void slow_increment(int id) 
// {
//     for (int i = 0; i < 3; ++i)
//     {
//         g_num_mutex.lock();
//         int g_num_running = ++g_num;
//         g_num_mutex.unlock();
//         std::cout << id << " => " << g_num_running << '\n';
 
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//     }
// }
 
// int main()
// {
//     std::thread t1(slow_increment, 0);
//     std::thread t2(slow_increment, 1);
//     t1.join();
//     t2.join();
// }

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

class Counter {
    int count = 0;
    std::mutex mtx;

public:
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        // std::cout << "Thread " << std::this_thread::get_id() << " incrementing..." << std::endl;
        count++;
        std::cout << "Thread " << std::this_thread::get_id() << " incremented count to " << count << std::endl;
    }

    int getCount() {
        std::lock_guard<std::mutex> lock(mtx);
        return count;
    }
};

int main() {
    Counter counter;
    std::vector<std::thread> threads;

    // Create 10 threads, each incrementing 1000 times
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 5; j++) {
                counter.increment();
            }
        });
    }

    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Final count: " << counter.getCount() << std::endl;
    return 0;
}