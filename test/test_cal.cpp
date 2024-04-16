#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "ThreadPool.hpp"

double monteCarloIntegrationThreadPool(int num_samples, int num_threads) {
    ThreadPool pool(num_threads);
    pool.init();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 1.0);

    std::vector<std::future<double>> samples(num_samples);
    double sum = 0.0;
    for (int i = 0; i < num_samples; ++i) {
        samples[i] = pool.submit([&dis, &gen]() {
            double x = dis(gen);
            return x * x;
        });
    }

    for (auto& sample : samples) {
        sum += sample.get();
    }
    pool.shut();
    return sum / num_samples;
}

int main() {
    int num_samples = 1000000;
    int num_threads = 10; // 假设有 10 个线程

    auto start = std::chrono::steady_clock::now();
    double result = monteCarloIntegrationThreadPool(num_samples, num_threads);
    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "ThreadPool Monte Carlo integration result: " << result << std::endl;
    std::cout << "Time taken: " << elapsed_seconds.count() << "s" << std::endl;


    return 0;
}
