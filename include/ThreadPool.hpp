#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <functional>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    void enqueue_task(std::function<void()> task);

private:
    size_t m_num_threads;
    std::queue<std::function<void()>> m_task_queue;
    std::vector<std::thread> m_workers;
    // bool m_shutdown = false;

    void worker_thread_loop();
};

#endif // THREAD_POOL_HPP