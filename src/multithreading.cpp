// MULTITHREADING BASICS
// ======================
// Key concepts:
//   1. std::thread: create and join/detach threads
//   2. std::mutex + std::lock_guard: protect shared data from data races
//   3. std::condition_variable: wait for conditions (producer-consumer)
//   4. std::promise/future: pass data between threads
//   5. std::async: launch async tasks with std::future for results

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <vector>

// --- 1. Basic thread ---
void basic_thread() {
    std::cout << "=== Basic Thread ===\n";
    std::thread t([]{ std::cout << "  Hello from thread!\n"; });
    t.join();  // must join or detach before thread object is destroyed
}

// --- 2. Mutex: protect shared data ---
std::mutex mtx;
int shared_counter = 0;

void increment() {
    for (int i = 0; i < 1000; ++i) {
        std::lock_guard<std::mutex> lock(mtx);  // RAII: locked on construct, unlocked on destruct
        shared_counter++;
    }
}

void mutex_demo() {
    std::cout << "\n=== Mutex (10 threads × 1000 increments) ===\n";
    shared_counter = 0;
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) threads.emplace_back(increment);
    for (auto& t : threads) t.join();
    std::cout << "  result: " << shared_counter << " (expected 10000)\n";
}

// --- 3. Condition variable: producer-consumer ---
std::condition_variable cv;
bool data_ready = false;

void condition_demo() {
    std::cout << "\n=== Condition Variable ===\n";
    data_ready = false;
    std::thread worker([]{
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return data_ready; });  // blocks until predicate is true
        std::cout << "  Worker: processing done\n";
        data_ready = false;
        lock.unlock();
        cv.notify_one();
    });
    {
        std::lock_guard<std::mutex> lock(mtx);
        data_ready = true;
        std::cout << "  Main: data ready\n";
    }
    cv.notify_one();
    { std::unique_lock<std::mutex> lock(mtx); cv.wait(lock, []{ return !data_ready; }); }
    worker.join();
}

// --- 4. Promise/Future: pass values between threads ---
void promise_demo() {
    std::cout << "\n=== Promise/Future ===\n";
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();
    std::thread t([](std::promise<int> p){ p.set_value(42); }, std::move(prom));
    std::cout << "  result: " << fut.get() << "\n";  // blocks until value is set
    t.join();
}

// --- 5. std::async: launch async tasks ---
void async_demo() {
    std::cout << "\n=== std::async ===\n";
    auto fut = std::async([]{ return 42; });  // launches on new thread (or deferred)
    std::cout << "  result: " << fut.get() << "\n";
}

int main() {
    basic_thread();
    mutex_demo();
    condition_demo();
    promise_demo();
    async_demo();
}
