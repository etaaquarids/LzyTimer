#include <concepts>
#include <format>
#include <iostream>
#include <chrono>
#include <future>
#include "platform/LzyPerciseSleep.h"

namespace {
    auto WelfordOnlineMean(auto x, auto x_mean, auto n) {
        return x_mean + static_cast<double>(x - x_mean) / (n + 1);
    }

    void print(std::convertible_to<std::string> auto fmt, auto&&... args) {
        std::cout << std::vformat(fmt, std::make_format_args(std::forward<decltype(args)>(args)...));
    }
    void print(std::convertible_to<char> auto number) {
        std::cout << number << "\n";
    }

    using Clock = std::chrono::high_resolution_clock;

    double getProcessTime() {
        static DWORD pid = GetCurrentProcessId();
        static HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

        FILETIME creation, exit, kernel, user;
        GetProcessTimes(process, &creation, &exit, &kernel, &user);
        uint64_t t1 = (uint64_t(kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime;
        uint64_t t2 = (uint64_t(user.dwHighDateTime) << 32) | user.dwLowDateTime;
        return (t1 + t2) / 1e7;
    }
}
namespace Benchmark {
    void cpuUsageFPSBenchmark(auto sleep_for, int tick = 128, int sample_size = 50) {
        
        auto duration = std::chrono::nanoseconds(static_cast<uint64_t>((1.0 / tick) * 1e9));
        double startProcess = getProcessTime();
        auto start = Clock::now();
       
        for (int i = 0; i < sample_size; ++i) {
            sleep_for(duration);
        }

        double endProcess = getProcessTime();
        double duration_in_second = (Clock::now() - start).count() / 1e9;
        double cpuUsage = 100 * (getProcessTime() - startProcess) / duration_in_second;

        print("cpu usage: {:.3f}% in sample_size {}\n", cpuUsage, sample_size);
    }

    void accuracyFPSBenchmark(auto sleep_for, int tick = 128, int sample_size = 50) {

        auto average = 0.0;
        auto average_delta = 0.0;
        auto variance = 0.0;
        auto duration = std::chrono::nanoseconds(static_cast<uint64_t>((1.0 / tick) * 1e9));
        auto last_tick_time = Clock::now();
        for (size_t i = 1; i <= sample_size; ++i) {
            auto start = Clock::now();
            sleep_for(duration);
            auto end = Clock::now();
            
            auto time_spend = (end - start).count();
            average = WelfordOnlineMean(time_spend, average, i);
              
            auto delta = time_spend - duration.count();
            average_delta = WelfordOnlineMean(delta, average_delta, i);
            variance = variance + (delta * delta);
        }
        auto stddev = sqrt(variance / (sample_size - 1));
        print("average frame time: {}us\n", average / 1e3);
        print("average frame time delta: {}us\n", average_delta / 1e3);
        print("accuracy: {}%\n", (1.0 - stddev / duration.count()) * 100);
    }
}
int main() {
    constexpr auto sample_size = 12;
    constexpr auto fps = 128;

    print("system sleep:\n");
    print("--- {} fps game loop test ---\n", fps);
    Benchmark::cpuUsageFPSBenchmark([](auto duration) { std::this_thread::sleep_for(duration); }, fps, sample_size);
    Benchmark::accuracyFPSBenchmark([](auto duration) { std::this_thread::sleep_for(duration); }, fps, sample_size);

    print("timer sleep:\n");
    print("--- {} fps game loop test ---\n", fps);
    Benchmark::cpuUsageFPSBenchmark([](auto duration) { Lzy::sleep_for(duration); }, fps, sample_size);
    Benchmark::accuracyFPSBenchmark([](auto duration) { Lzy::sleep_for(duration); }, fps, sample_size);

	return 0;
}