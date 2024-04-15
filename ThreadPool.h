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


class Task {
public:
    virtual void execute() = 0;
    virtual ~Task();
};

class RunnableTask : public Task {
    std::function<void()> func;
public:
    RunnableTask(std::function<void()> func);
    void execute();
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

    Task* stealTask(int cur);
    void worker(int idx);
    int random();

public:
    ThreadPool(int numThreads);
    ~ThreadPool();
    void submit(std::function<void()> func);

};


