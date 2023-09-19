// LzyTimer.cpp: 定义应用程序的入口点。
//

#include "LzyTimer.hpp"
#include <format>

#include <iostream>
using Clock = std::chrono::high_resolution_clock;
auto start_time = Clock::now();

void print(std::convertible_to<std::string> auto fmt, auto&&... args) {
	std::cout << std::vformat(fmt, std::make_format_args(std::forward<decltype(args)>(args)...));
}
void print(std::convertible_to<char> auto number) {
	std::cout << number << "\n";
}

struct Test {
	std::vector<int> i;
	Test(int i) :i(i) {
		std::cout << "create constructor is called\n";
	}
	Test(Test&& t) noexcept : i(std::move(t.i))  {
		std::cout << "move constructor is called\n";
	}
	Test(const Test& t) : i(t.i) {
		std::cout << "copy constructor is called\n";
	}


};
std::chrono::microseconds now() {
	return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start_time);
}
int main()
{
	using namespace std::literals::chrono_literals;
	Lzy::Timer::Timer timer(10ms);
	
	
	timer.setTimeout([]() { print("[{}] work by {}\n", now(), 500ms); }, 500ms);
	timer.setTimeout([]() { print("[{}] work by {}\n", now(), 600ms); }, 600ms);
	timer.setTimeout([&]() { 
		print("[{}] work by {}\n", now(), 1000ms);
		timer.setTimeout([]() {print("[{}] work by {}\n", now(), 1000ms); }, 1000ms);
	}, 1000ms);

	timer.setTimeout([]() { print("[{}] work by {}\n", now(), 3000ms); }, 3000ms);
	timer.setTimeout([]() { print("[{}] work by {}\n", now(), 4000ms); }, 4000ms);
	timer.setTimeout([]() { print("[{}] work by {}\n", now(), 5000ms); }, 5000ms);
	timer.setTimeout([]() { print("[{}] work by {}\n", now(), 6000ms); }, 6000ms);
	int u;
	timer.start();
	while (true) {
		std::cin >> u;
		auto duration = std::chrono::milliseconds(u);
		auto last = now();
		timer.setTimeout([=]() { print("{} work by {}\n", duration, now() - last); }, duration);
	}
	std::cout << "Hello CMake." << std::endl;
	return 0;
}
