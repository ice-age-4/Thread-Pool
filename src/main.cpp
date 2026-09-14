#include <iostream>
#include <thread>
#include "ThreadPool.hpp"
#include <windows.h>


int main () {
    std::cout << "Hello, Project!" << std::endl;
    ThreadPool pool(4);
    for (int i = 0; i < 10; ++i) {
        pool.enqueue_task([i] {
            std::cout << "Task " << i << "\nC++ thread: " << std::this_thread::get_id() << "\nWindows thread: " << GetCurrentThreadId() << "\nCPU: " << GetCurrentProcessorNumber() << std::endl;
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(60)); 
    return 0;
}