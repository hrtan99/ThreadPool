#pragma once
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
#include <future>

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
    void execute() {
        wrapper->run();
    }
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
    std::atomic<bool> start, stop;
    std::condition_variable cv;
    std::uniform_int_distribution<> distrib;
    std::unordered_map<std::thread::id, int> idMap;
    std::mutex mutex;
    int threshold;

    Task* stealTask(int cur) {
        for (int idx = random() % threads.size(), i = 0; i < threads.size(); idx = (idx + 1) % threads.size(), ++i) {
            if (idx != cur && !queues[idx].empty()) {
                Task* task = nullptr;
                {
                    std::lock_guard<std::mutex> lock(queues_lock[idx]);
                    if (!queues[idx].empty()) {
                        task = queues[idx].back();
                        queues[idx].pop_back();
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

        if (!start) {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock);
        }

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
    ThreadPool(int numThreads) : start(false), stop(false), distrib(0, numThreads - 1), queues_lock(numThreads), cvs(numThreads), threshold(numThreads) {

    }

    ~ThreadPool() {
        if (!stop) shut();
    }

    void init() {
        for (int i = 0; i < threshold; ++i) {
            queues.emplace_back();
            threads.emplace_back(std::thread([this, i] {
                idMap[std::this_thread::get_id()] = i;
                worker(i);
            }));
        }
        start = true;
        cv.notify_all();
    }

    void shut() {
        stop = true;
        for (int i = 0; i < threads.size(); ++i) {
            cvs[i].notify_all();
            threads[i].join();
        }
    }

    template<typename FuncType>
    std::future<std::result_of_t<FuncType()>> submit(FuncType&& func) {
        using result_type = std::result_of_t<FuncType()>;
        std::packaged_task<result_type()> task(std::move(func));
        std::future<result_type> res(task.get_future());
        int idx = random();
        {
            std::lock_guard<std::mutex> lock(queues_lock[idx]);
            queues[idx].push_back(new RunnableTask(std::move(task)));
        }
        cvs[idx].notify_all();
        return res;
    }

    template<typename FuncType, typename... Args>
    std::future<std::invoke_result_t<FuncType, Args...>> submit(FuncType&& func, Args&&... args) {
        using result_type = std::invoke_result_t<FuncType, Args...>;
        std::packaged_task<result_type()> task(std::bind(std::forward<FuncType>(func), std::forward<Args>(args)...));
        std::future<result_type> res(task.get_future());
        int idx = random();
        {
            std::lock_guard<std::mutex> lock(queues_lock[idx]);
            queues[idx].push_back(new RunnableTask(std::move(task)));
        }
        cvs[idx].notify_all();
        return res;
    }

};


