#include "ThreadPool.h"
void test() {
    {
        std::mutex mutex;
        ThreadPool pool(10);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 100);
        int numTasks = 30;
        std::vector<std::future<int>> results(numTasks);
        for (int i = 0; i < numTasks; ++i) {
            int sleepTime = distrib(gen);
            results[i] = pool.submit([i, sleepTime, &mutex] {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
                std::lock_guard<std::mutex> lock(mutex);
                std::cout << "Task " << i << " executed by thread " << std::this_thread::get_id() << std::endl << std::flush;
                return i;
            });
        }
        int total = 0;
        for (auto& res : results) {
            total += res.get();
            
        }
        std::cout << "total is: " << total << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    std::cout << "All tasks completed, main thread exiting." << std::endl;
}

int main() {
    test();
    return 0;
}