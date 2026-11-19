#pragma once

#include <functional>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

class ThreadPool
{
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    void enqueue_task(std::function<void()> task);
	void shutdown();

private:
    std::mutex m_queue_mutex;
    std::condition_variable cv;
    bool m_shutdown = false;
    std::queue<std::function<void()>> m_task_queue;
    std::vector<std::thread> m_workers;
    

    void worker_thread_loop();
};