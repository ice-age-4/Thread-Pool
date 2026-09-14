#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t numThreads)
    : m_num_threads(numThreads)
{
    m_workers.reserve(m_num_threads);
    for (size_t i = 0; i < m_num_threads; ++i) { // i++ ?
        m_workers.emplace_back(&ThreadPool::worker_thread_loop, this);
    }
}

ThreadPool::~ThreadPool()
{
    // TODO: Implement destructor
}

void ThreadPool::enqueue_task(std::function<void()> task)
{
    m_task_queue.push(std::move(task));
}

void ThreadPool::worker_thread_loop()
{
    // while (!m_shutdown) {
    while (true) {
        if (m_task_queue.empty()) {
            continue;
        }

        std::function<void()> task = m_task_queue.front();
        m_task_queue.pop();
        task();
    }
}
