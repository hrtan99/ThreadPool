#include <iostream>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <map>
#include <random>

std::mutex mutex;

class Task {
public:
    virtual void execute() = 0;
    virtual ~Task() {}
};

class RunnableTask : public Task {
    std::function<void()> func;
public:
    RunnableTask(std::function<void()> func) : func(func) {}
    void execute() override {
        func();
    }
};

class ThreadPool {
    std::vector<std::thread> threads;
    std::vector<std::deque<Task*>> queues;
    std::vector<std::mutex> queues_lock;
    std::vector<std::condition_variable> cvs;
    std::mutex mutex_;
    std::atomic<bool> stop;
    std::uniform_int_distribution<> distrib;
    std::unordered_map<std::thread::id, int> idMap;

    Task* stealTask(int cur) {
        for (int idx = random() % threads.size(), i = 0; i < threads.size(); idx = (idx + 1) % threads.size(), ++i) {
            if (idx != cur && !queues[idx].empty()) {
                Task* task = nullptr;
                {
                    std::lock_guard<std::mutex> lock(queues_lock[idx]);
                    if (!queues[idx].empty()) {
                        task = queues[idx].back();
                        queues[idx].pop_back();
                        // std::lock_guard<std::mutex> lock(mutex);
                        // std::cout << "Stealed Task in " << cur << "\n";
                    }
                }
                if (task) {
                    return task;
                }
            }
        }
        return nullptr;
    }

    void worker(int idx) {

        while (!stop) {
            Task* task = nullptr;
            {
                std::lock_guard<std::mutex> lock(queues_lock[idx]);
                if (!queues[idx].empty()) {
                    task = queues[idx].front();
                    queues[idx].pop_front();
                }
            }
            if (!task) {
                task = stealTask(idx);
            }
            if (task) {
                task->execute();
                delete task;
            }
            else {
                std::unique_lock<std::mutex> lock(queues_lock[idx]);
                cvs[idx].wait(lock);
            }
        }
    }

    int random() {
        std::random_device rd;
        std::mt19937 gen(rd());
        return distrib(gen);
    }

public:
    ThreadPool(int numThreads) : stop(false), distrib(0, numThreads - 1), queues_lock(numThreads), cvs(numThreads) {
        for (int i = 0; i < numThreads; ++i) {
            queues.emplace_back();
            threads.emplace_back(std::thread([this, i] {
                idMap[std::this_thread::get_id()] = i;
                worker(i);
            }));
        }
    }

    void submit(std::function<void()> func) {
        int idx = random();
        {
            std::lock_guard<std::mutex> lock(queues_lock[idx]);
            queues[idx].push_back(new RunnableTask(func));
            cvs[idx].notify_all();
        }
    }

    ~ThreadPool() {
        stop = true;
        for (int i = 0; i < threads.size(); ++i) {
            cvs[i].notify_all();
            threads[i].join();
        }
    }
};

void test() {
    {
        ThreadPool pool(10);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 100);
        for (int i = 0; i < 1000; ++i) {
            int sleepTime = distrib(gen);
            pool.submit([i, sleepTime] {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
                std::lock_guard<std::mutex> lock(mutex);
                std::cout << "Task " << i << " executed by thread " << std::this_thread::get_id() << std::endl << std::flush;
            });
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    std::cout << "All tasks completed, main thread exiting." << std::endl;
}

int main() {
    test();
    return 0;
}

