#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <stdexcept>
#include "ThreadPool.hpp"


// ============ Ctor Tests ============

// default ctor: creates a pool with std::thread::hardware_concurrency() threads
TEST(ThreadPoolCtorTest, DefaultCtorMatchHWConcurrency) {
    ThreadPool pool{};
    EXPECT_EQ(pool.get_num_threads(), std::max(1u, std::thread::hardware_concurrency()));
}

// ctor with argument 0: creates a pool with 1 thread and tasks can run
TEST(ThreadPoolCtorTest, ZeroThreadsFallsBackToOne) {
    std::atomic<bool> ran{ false };
    {
        ThreadPool pool{ 0 };
        EXPECT_EQ(pool.get_num_threads(), 1u);

        pool.enqueue_task([&ran] { ran = true; });
    }
    EXPECT_TRUE(ran);
}


// ============ Basic Usage Tests ============

// single task is able to run on  a pool
TEST(ThreadPoolBasicTest, SingleTaskRuns) {
    std::atomic<bool> ran{ false };
    {
        ThreadPool pool{ 4 };
        pool.enqueue_task([&ran] { ran = true; });
    }
    EXPECT_TRUE(ran);
}

// a task can assign another task (before shutdown).    [for after shutdown: see ConcurrencyTest: TaskEnqueueTaskAfterShutdown]
TEST(ThreadPoolBasicTest, TaskAssignTaskBeforeShutdown) {
    std::atomic<int> counter{ 0 };
    {
        ThreadPool pool{};
        pool.enqueue_task([&pool, &counter] {
            for (int i = 0; i < 10; ++i) {
                pool.enqueue_task([&counter] { ++counter; });
            }
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(300));    // wait for the task to finish assign tasks
    }  // destruction here
    EXPECT_EQ(counter, 10);
}


// ============ Dtor & Shutdown Tests ============

// on destruction: a pool waits for all task to finish (drain).
TEST(ThreadPoolDtorTest, RunsAllTasksBeforeDestruction) {
    std::atomic<int> counter{ 0 };
    {
        ThreadPool pool{ 4 };
        for (int i = 0; i < 10; ++i) {
            pool.enqueue_task([&counter] { 
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                ++counter; });
        }
    }  // destruction here
    EXPECT_EQ(counter, 10);
}

// enqueue tasks after shutdown throws
TEST(ThreadPoolDtorTest, EnqueueAfterShutdownThrows) {
    std::atomic<int> counter{ 0 };
    {
        ThreadPool pool{ 4 };
        for (int i = 0; i < 10; ++i) {
            pool.enqueue_task([&counter] { ++counter; });
        }
        pool.shutdown();
        EXPECT_THROW(pool.enqueue_task([ &counter ] { ++counter; }), std::runtime_error);
    }
}

// properly manage multiple shutdown calls and destruction. no crash or deadlock
TEST(ThreadPoolDtorTest, ShutdownTwiceThenDtor) {
    std::atomic<int> counter{ 0 };
    {
        ThreadPool pool{ 4 };
        for (int i = 0; i < 10; ++i) {
            pool.enqueue_task([ &counter ] { ++counter; });
        }
        pool.shutdown();
        pool.shutdown();
	}  // destruction here
    // if flow reached here, everything is fine
    SUCCEED();
}

TEST(ThreadPoolDtorTest, ConcurrentShutdownCallersAllWaitForCompletion) {
    std::atomic<int> counter{ 0 };
    std::atomic<int> returned_early{ 0 };
    {
        ThreadPool pool{ 4 };
        for (int i = 0; i < 20; ++i) {
            pool.enqueue_task([&counter] {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                ++counter;
                });
        }

        std::vector<std::thread> callers;
        for (int i = 0; i < 4; ++i) {
            callers.emplace_back([&pool, &counter, &returned_early] {
                pool.shutdown();
                if (counter != 20) ++returned_early;   // shutdown returned before everything finished
                });
        }
        for (auto& t : callers) t.join();   // join before the pool is destroyed (see the lifetime rule)
    }
    EXPECT_EQ(returned_early, 0);
    EXPECT_EQ(counter, 20);
}


// ============ Edge Cases Tests ============

// pool with no tasks doesn't get stuck
TEST(ThreadPoolEdgeCasesTest, EmptyTasksQueue) {
    {
        ThreadPool pool{ 4 };
    }
    // if flow reached here, everything is fine
    SUCCEED();
}

// empty task passes
TEST(ThreadPoolEdgeCasesTest, EnqueueEmptyTask) {
    {
        ThreadPool pool{ 4 };
        pool.enqueue_task([] {});
    }
    // if flow reached here, everything is fine
    SUCCEED();
}

// TODO: task that captures move-only var

// TODO: task that throws


// ============ Concurrency Tests ============


// test thread-safety of enqueue_task()
TEST(ThreadPoolConcurrencyTest, MultipleThreadsEnqueuingTasks) {
    std::atomic<int> counter{ 0 };
    {
        ThreadPool pool{};

        std::vector<std::thread> threads;

        // 10 independent threads enqueuing 1000 tasks each
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([ &pool, &counter ] {
                for (int j = 0; j < 1000; ++j) {
                    pool.enqueue_task([ &counter ] { ++counter; });
                }

                });
        }

        for (std::thread& t : threads) {
            t.join();
        }

    }  // destruction here. all threads joined - no task enqueuing after destruction.

    EXPECT_EQ(counter, 10000);
}

// the pool actually runs a task on a other thread than the one of the main program.
TEST(ThreadPoolConcurrencyTest, TaskRunsOnOtherThreadThanMain) {
    std::atomic<std::thread::id> thread_id{};

    {
        ThreadPool pool{ 4 };
        pool.enqueue_task([&thread_id] { thread_id = std::this_thread::get_id(); });
    }
	EXPECT_NE(thread_id, std::thread::id{}) << "Task did not run";  // if passed, task ran
	EXPECT_NE(thread_id, std::this_thread::get_id()) << "Task ran on the same thread as the main thread";  // if passed, task ran on a different thread than the main thread
}

// all workers are busy and incoming tasks waiting in the queue and finally executed
TEST(ThreadPoolConcurrencyTest, TasksWaitInQueue) {
    std::atomic<int> counter{ 0 };

    {
        ThreadPool pool{};
        size_t pool_size = pool.get_num_threads();
        for (size_t i = 0; i < pool_size; ++i) {
            pool.enqueue_task([] { std::this_thread::sleep_for(std::chrono::milliseconds(300)); });
        }

        for (int i = 0; i < 100; ++i) {
            pool.enqueue_task([ &counter ] { ++counter; });
        }
    }
    EXPECT_EQ(counter, 100);
}
