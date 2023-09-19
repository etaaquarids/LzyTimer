// LzyTimer.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once


#include <thread>
#include <future>
#include <functional>
#include <vector>
#include <list>
#include <algorithm>
#include <concepts>
#include "platform/LzyPerciseSleep.h"

namespace Lzy::Timer {
	namespace concepts {
		template<typename AnyType>
		concept chrono_time = requires(AnyType any) {
			std::chrono::duration_cast<std::chrono::milliseconds>(any);
		};

		template<typename AnyType>
		concept timer_executor = requires() {
			AnyType::execute();
		};
	}
	namespace {
		template<typename T>
		struct TimeWheel {
		private:
			std::vector<std::list<T>> wheel;
		public:
			size_t tick{ 0 };
			TimeWheel(size_t size): wheel(size){	
			}

			std::list<T>& operator[] (size_t index) {
				return wheel[index];
			}

			bool tick_once(auto&& callback) {
				++tick %= wheel.size();
			
				std::ranges::for_each(wheel[tick], callback);
				wheel[tick].clear();
				return tick == 0;
			}

			void setTimeout(T&& fn, int tick_id) {
				wheel[tick].emplace_back(fn);
			}
			// all before tick has been used;
			auto begin() {
				return wheel.begin() + tick;
			}
			auto end() {
				return wheel.end();
			}
			auto size() {
				return wheel.size();
			}
		};

		// must be exponential multiple of 2, for shift optimization
		// 128tick and 2^12 will keep timer in 25s in wheel 1;
		template<typename T>
		struct TimeWheels {
			struct TimeSlot {
				T fn;
				uint64_t tick;
			};

			TimeWheel<T> driving_wheel;
			std::vector<TimeWheel<TimeSlot>> driven_wheels;
			std::vector<size_t> wheel_sizes;

			TimeWheels(std::unsigned_integral auto driving_wheel_sizes_input, std::unsigned_integral auto... driven_wheel_sizes_input) :
				wheel_sizes{ driving_wheel_sizes_input, driven_wheel_sizes_input... },
				driving_wheel{ driving_wheel_sizes_input },
				driven_wheels{ driven_wheel_sizes_input... }
			{
			}

			void move_to_wheel_n(TimeSlot&& time_slot, const std::invocable<T> auto& callback, size_t wheel_id) {
				if (wheel_id == 0) {
					if (time_slot.tick == 0) {
						callback(time_slot.fn);
					}
					else {
						driving_wheel[time_slot.tick].emplace_back(std::move(time_slot.fn));
					}
				}
				else {
					auto tick_id = time_slot.tick / wheel_sizes[wheel_id - 1];
					time_slot.tick = time_slot.tick % wheel_sizes[wheel_id - 1];
					if (tick_id == 0) {
						move_to_wheel_n(std::move(time_slot), callback, wheel_id - 1);
					}
					else {
						driven_wheels[wheel_id - 1][tick_id].emplace_back(std::move(time_slot));
					}
				}
			}
			void tick_once(const std::invocable<T> auto& callback) {
				if (!driving_wheel.tick_once(callback)) {
					return;
				};
				for (int i = 0; driven_wheels[i].tick_once([&](TimeSlot& ts) { move_to_wheel_n(std::forward<TimeSlot>(ts), callback, i); }); i++);
			}


			// will crash when tick is bigger than capticaty
			void add_by_tick(T&& fn, uint64_t tick_num) {
				auto saving_wheel_tick = std::vector<uint64_t>(wheel_sizes.size(), 0);
				int i = 0;

				saving_wheel_tick[i] = (tick_num + driving_wheel.tick) % wheel_sizes[i];
				if (tick_num < wheel_sizes[i]) {
					driving_wheel[saving_wheel_tick[i]].emplace_back(std::forward<T>(fn));
					return;
				}
				tick_num = tick_num / wheel_sizes[i];
			
				for (size_t i = 1;; i++) {
					saving_wheel_tick[i] = (tick_num + driven_wheels[i - 1].tick) % wheel_sizes[i];
					if (tick_num < wheel_sizes[i]) {
						uint64_t save_tick = saving_wheel_tick[0];

						for (size_t j = 1; j < i; j++)
						{
							save_tick += saving_wheel_tick[j] * wheel_sizes[j - 1];
						}
						driven_wheels[i - 1][saving_wheel_tick[i]].emplace_back(std::forward<T>(fn), save_tick);
						break;
					}
					tick_num = tick_num / wheel_sizes[i];
				}

			}

		};

		constexpr auto driving_wheel_size = 128u * 64u;
		constexpr auto driven_wheel_size = 64u;

		struct std_async {
			static void execute(auto&& any) {
				auto future = std::async(std::launch::async, std::move(any));
			}
		};
	}

	template<
		typename Clock = std::chrono::high_resolution_clock,
		std::invocable Callable = std::function<void()>,
		typename concepts::executor = Lzy::Timer::std_async
	>
	struct Timer {
		using time_point = std::chrono::time_point<Clock>;

	private:
		TimeWheels<Callable> time_wheels{ driving_wheel_size, driven_wheel_size, driven_wheel_size, driven_wheel_size };
		std::chrono::microseconds tick_time;
		Clock::time_point last_tick_time;

	public:
		Timer(std::chrono::milliseconds tick_time, ) :tick_time(tick_time) {
		}
	public:
		void start(const std::invocable<Callable> auto& callback) {
			std::thread([this]() {
				while (true) {
					Lzy::sleep_for(tick_once(callback) - Clock::now());
				}
			}).detach();
		}

		void setTimeout(std::invocable auto&& fn, const concepts::chrono_time auto& duration) {
			if (duration < tick_time) {
				executor::execute(fn);
			}
			else {
				auto tick_num = (duration / tick_time);
				time_wheels.add_by_tick(std::move(fn), tick_num);
			}
		}

		// return when to tick_next;
		Clock::time_point tick_once(const std::invocable<Callable> auto& callback) {
			static auto last_tick_time = Clock::now();
			last_tick_time += tick_time;
			time_wheels.tick_once([](const auto& fn) { callback(fn); });
			return last_tick_time;
		}
	}; 
}