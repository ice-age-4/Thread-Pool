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

	// unavailable copy and move constructors and assignment operators for ThreadPool, as it has uncopyable and unmovable members.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    void enqueue_task(std::function<void()> task);
	void shutdown();

	[[nodiscard]] size_t get_num_threads() const noexcept { return m_workers.size(); }

private:
    std::mutex m_queue_mutex;
    std::condition_variable cv;
    bool m_shutdown = false;
    std::queue<std::function<void()>> m_task_queue;
    std::vector<std::thread> m_workers;
    

    void worker_thread_loop();
};