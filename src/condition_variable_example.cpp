// CONDITION VARIABLE
// ===================
// Main point: std::condition_variable lets threads wait for a condition.
// Always use with std::unique_lock and a predicate (spurious wakeups).

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;
bool ready = false;

void worker_thread() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, []{ return ready; });  // blocks until ready==true (handles spurious wakeups)
    std::cout << "Worker: processing data\n";
    ready = false;
    lock.unlock();
    cv.notify_one();
}

int main() {
    std::thread worker(worker_thread);
    { std::lock_guard<std::mutex> lock(mtx); ready = true; }
    cv.notify_one();  // wake worker
    { std::unique_lock<std::mutex> lock(mtx); cv.wait(lock, []{ return !ready; }); }
    worker.join();
}
