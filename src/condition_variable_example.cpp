#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;
bool ready = false;

void worker_thread() {
    // Wait until main() sends data
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [] { return ready; });

    // After the wait, we own the lock.
    std::cout << "Worker thread is processing data\n";

    // Send data back to main()
    ready = false;
    std::cout << "Worker thread signals data processing completed\n";

    // Manual unlocking is done before notifying, to avoid waking up
    // the waiting thread only to block again (see notify_one for details)
    lock.unlock();
    cv.notify_one();
}

int main() {
    std::thread worker(worker_thread);

    // Send data to the worker thread
    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
        std::cout << "main() signals data ready for processing\n";
    }
    cv.notify_one();

    // Wait for the worker
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !ready; });
    }

    worker.join();

    return 0;
}
