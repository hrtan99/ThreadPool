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
    virtual ~Task() { }
};

class RunnableTask : public Task {

    struct WrapperBase {
        virtual void run() = 0;
        virtual ~WrapperBase() { };
    };

    template<class F>
    struct WrapperImpl : WrapperBase {
        F func;
        WrapperImpl(F&& f) : func(std::move(f)) { }
        void run() override { func(); }
    };

    std::shared_ptr<WrapperBase> wrapper;

public:
    template<class F>
    RunnableTask(F&& f) : wrapper(new WrapperImpl(std::move(f))) { }
    void execute();
    void operator()() { wrapper->run(); }
    RunnableTask(RunnableTask&& other) : wrapper(std::move(other.wrapper)) { }
    RunnableTask& operator=(RunnableTask&& other) {
        wrapper = std::move(other.wrapper);
        return *this;
    }
    RunnableTask(const RunnableTask&) = delete;
    RunnableTask(RunnableTask&) = delete;
    RunnableTask& operator=(const RunnableTask&) = delete;
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


