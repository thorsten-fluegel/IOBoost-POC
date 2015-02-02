#include <iostream>
#include <chrono>
#include <thread>


std::chrono::duration<double> time(std::function<void ()> f)
{
	auto t_start = std::chrono::high_resolution_clock::now();

	f();

	auto t_end = std::chrono::high_resolution_clock::now();
	auto duration_s = std::chrono::duration_cast<std::chrono::duration<double>>(t_end - t_start);

	return duration_s;
}


void wmain(int argc, wchar_t** argv)
{
	auto sleep_1s = []() {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	};

	std::wcout << L"slept for " << time(sleep_1s).count() << L"s" << std::endl;
}
