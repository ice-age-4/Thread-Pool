#include <iostream>
#include <thread>
#include "ThreadPool.hpp"
#include <windows.h>

constexpr size_t TASKS_COUNT = 18;
int main () {
    std::cout << "Hello, Project!" << std::endl;
    ThreadPool pool{};

	std::cout << "Number of threads in the pool: " << pool.get_num_threads() << std::endl;

	std::vector<int> cores(std::thread::hardware_concurrency());
	std::mutex cout_mutex;
    for (size_t i = 0; i < TASKS_COUNT; ++i) {
        pool.enqueue_task([i, &cout_mutex, &cores]() {

            size_t core_index = GetCurrentProcessorNumber();
            ++cores[core_index];
            
			//std::scoped_lock lock(cout_mutex);
            //std::cout << "Task " << i << "\nC++ thread: " << std::this_thread::get_id() << "\nWindows thread: " << GetCurrentThreadId() << "\nCPU: " << core_index << "\n------\n" << std::endl;

            return 0;
        });
    }

    //std::this_thread::sleep_for(std::chrono::seconds(2));
	pool.shutdown();
	for (size_t i = 0; i < cores.size(); ++i) {
		std::cout << "Tasks executed on core " << (i) << ": " << cores[i] << std::endl;
	}
    return 0;
}
