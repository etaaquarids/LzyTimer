#pragma once
#include <chrono>
#include <timeapi.h>
#include <WinBase.h>
#include <cstdint>
#pragma (lib "Winmm.lib")

namespace Lzy {
    namespace {
        auto WelfordOnlineVariance(std::chrono::nanoseconds x_n, auto& M2, std::chrono::nanoseconds& mean_of_x_n, auto& n) {
            n += 1;
            auto delta_1 = x_n - mean_of_x_n;
            mean_of_x_n += (delta_1 / n);
            auto delta_2 = x_n - mean_of_x_n;
            M2 += delta_1.count() * delta_2.count();
        }

        struct TimerResolutionSetter {
            TIMECAPS tc;
            DWORD timerRes;
            TimerResolutionSetter(DWORD timerRes = 15u) : timerRes(timerRes) {
                if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR) {
                    timeBeginPeriod(timerRes);
                }
            };
            ~TimerResolutionSetter() {
                timeEndPeriod(timerRes);
            }
        };
    }

    void sleep_for(std::chrono::nanoseconds seconds) {
        using Clock = std::chrono::high_resolution_clock;
        using namespace std::literals::chrono_literals;

        static TimerResolutionSetter trs;

        static HANDLE timer = CreateWaitableTimerA(NULL, FALSE, NULL);
        static auto estimate = 5000000ns;
        static auto mean = 5000000ns;
        static double m2 = 0;
        static int64_t count = 1;

        while (seconds - estimate > 1e2ns) {
            auto toWait = seconds - estimate;
            LARGE_INTEGER due{};
            due.QuadPart = -int64_t(toWait.count() * 1e-2);
            SetWaitableTimerEx(timer, &due, 0, NULL, NULL, NULL, 0);
            auto start = Clock::now();
            WaitForSingleObject(timer, INFINITE);
            auto end = Clock::now();

            auto observed = end - start;
            seconds -= observed;
            auto error = observed - toWait;

            WelfordOnlineVariance(error, m2, mean, count);
            uint64_t stddev = static_cast<uint64_t>(sqrt(m2 / (count - 1)));
            estimate = std::chrono::nanoseconds((uint64_t)mean.count() + stddev);
        }

        // spin lock
        auto start = Clock::now();
        while ((Clock::now() - start) < seconds);
    }
}