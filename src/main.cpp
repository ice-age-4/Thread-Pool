#include <iostream>
#include <thread>
#include "ThreadPool.hpp"
#include <windows.h>


int main () {
    std::cout << "Hello, Project!" << std::endl;
    ThreadPool pool{};

	std::mutex cout_mutex;
    for (int i = 0; i < 10; ++i) {
        pool.enqueue_task([i, &cout_mutex]() {
			std::scoped_lock lock(cout_mutex);
            std::cout << "Task " << i << "\nC++ thread: " << std::this_thread::get_id() << "\nWindows thread: " << GetCurrentThreadId() << "\nCPU: " << GetCurrentProcessorNumber() << "\n------\n" << std::endl;

            return 0;
        });
    }

    //std::this_thread::sleep_for(std::chrono::seconds(2));
	//pool.shutdown();
	std::cout << "That's it." << std::endl;
    return 0;
}
