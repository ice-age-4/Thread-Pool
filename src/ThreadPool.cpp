#include "ThreadPool.hpp"

#include <stdexcept>

ThreadPool::ThreadPool(size_t num_threads)
{
    if (num_threads == 0) {
		num_threads = 1; // Fallback to at least one thread if hardware_concurrency returns 0 (or if the user explicitly requests 0 threads).
		// log that hardware_concurrency returned 0 (or the user explicitly requested 0 threads) and we are falling back to 1 thread
    }
    m_workers.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        m_workers.emplace_back(&ThreadPool::worker_thread_loop, this);
    }
}


ThreadPool::~ThreadPool()
{
    shutdown();
}


void ThreadPool::enqueue_task(std::function<void()> task)
{
    {
        std::scoped_lock lock(m_queue_mutex);
        if (m_shutdown) {
            throw std::runtime_error("Cannot enqueue task on a shutdown ThreadPool");
        }
        m_task_queue.push(std::move(task));
    }
    cv.notify_one();
}


// Gracefully shuts down the pool: stops accepting tasks, lets workers finish everything already queued, and joins them.
// Thread-safe: every caller blocks until the workers have joined, so when it returns the pool is fully stopped.
// Must not be called from a task running in the pool (a worker would try to join itself).
void ThreadPool::shutdown()
{
    {
        std::scoped_lock lock(m_queue_mutex);
        m_shutdown = true;
    }
    cv.notify_all();

    std::scoped_lock join_lock(m_join_mutex);
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}


void ThreadPool::worker_thread_loop()
{
    while (true) {
		std::function<void()> task = nullptr;
        {
            // wait for a task to be available in the queue
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            cv.wait(lock, [this]() { return (!m_task_queue.empty() || m_shutdown); });

            // exit the thread loop if shutdown and no tasks left
            if (m_shutdown && m_task_queue.empty()) {
                return;
            }

            // get the next task from the queue within the lock
            task = std::move(m_task_queue.front());
            m_task_queue.pop();
        }

        // run the task
        task();
    }
}
