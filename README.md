# Thread-Pool

A small thread pool in modern C++17, built with CMake and covered by GoogleTest unit tests.

A fixed number of worker threads take tasks from a shared queue, using only `std::mutex` and `std::condition_variable`. Idle workers sleep (no busy-waiting), and shutdown is clean.

> **Status:** work in progress. See [Known limitations](#known-limitations) and [Roadmap](#roadmap).

## Features

- Fixed number of worker threads (default: `std::thread::hardware_concurrency()`; `0` becomes `1`)
- Thread-safe `enqueue_task()`, callable from any number of threads, including from inside a running task (before shutdown)
- Graceful shutdown: every task accepted before shutdown runs to completion
- `shutdown()` can be called more than once, and the destructor calls it automatically
- Enqueuing after shutdown throws `std::runtime_error`
- Non-copyable and non-movable
- Unit tests for construction, shutdown, edge cases, and concurrency

## Usage

```cpp
#include <atomic>
#include <iostream>
#include "ThreadPool.hpp"

int main() {
    std::atomic<int> counter{0};

    {
        ThreadPool pool{4};

        for (int i = 0; i < 100; ++i) {
            pool.enqueue_task([&counter] { ++counter; });
        }
    }   // the destructor waits for all tasks to finish

    std::cout << counter << '\n';   // 100
}
```

A larger example (parallel sum, printing the thread each chunk ran on) is in [`examples/example.cpp`](examples/example.cpp).

## API

| Member | Description |
|---|---|
| `ThreadPool(size_t num_threads = hardware_concurrency())` | Starts the worker threads. |
| `void enqueue_task(std::function<void()> task)` | Adds a task to the queue. Throws `std::runtime_error` if the pool was shut down. |
| `void shutdown()` | Thread-safe and idempotent. Stops accepting tasks, waits for all queued tasks to finish, and joins the workers. Every concurrent caller blocks until this is complete, so when it returns the pool is fully stopped. Must not be called from inside a task of the same pool. |
| `size_t get_num_threads() const` | Number of worker threads. |

## How it works

- A single `std::queue<std::function<void()>>` is protected by one mutex.
- `enqueue_task` pushes under the lock and calls `notify_one()`.
- Each worker waits on the condition variable with the predicate `!queue.empty() || shutdown`, so spurious wakeups are handled.
- The lock is released **before** the task runs, so tasks execute in parallel.
- `shutdown()` sets the flag **under the mutex** (avoiding a lost wakeup), calls `notify_all()`, and joins every worker. A worker exits only when shutdown has been requested **and** the queue is empty, which is what guarantees the drain behavior.
- The join phase is serialized by a second mutex, so concurrent `shutdown()` callers all wait until the workers have actually joined.

## Thread safety and lifetime

`enqueue_task()` and `shutdown()` can be called from any thread. The pool must outlive every one of these calls: destroying it while another thread is still inside `enqueue_task()` or `shutdown()` is a use-after-free (undefined behavior).

To make sure of that, either:
- join the threads that use the pool before the pool is destroyed, or
- share ownership of the pool between those threads with `std::shared_ptr<ThreadPool>`.

If you use `std::shared_ptr`, don't let the last reference be released from inside a task running on the pool itself. The destructor would then run on a worker thread and try to join itself.

## Building

Requires CMake 3.20+ and a C++17 compiler. GoogleTest is downloaded automatically on the first configure.

Developed with Visual Studio on Windows: open the repository with *File → Open → Folder*, build, and run the tests from Test Explorer.

> Tested with MSVC on Windows.

## Project layout

```
CMakeLists.txt
include/    ThreadPool.hpp
src/        ThreadPool.cpp
examples/   example.cpp
tests/      thread_pool_tests.cpp
```

CMake targets:

| Target | Type | Purpose |
|---|---|---|
| `thread_pool` | static library | The pool itself |
| `example` | executable | Parallel-sum usage example |
| `thread_pool_tests` | executable | GoogleTest suite |

## Tests

Tests live in `tests/thread_pool_tests.cpp`, grouped into suites:

| Suite | What it covers |
|---|---|
| `ThreadPoolCtorTest` | Default thread count, `0` threads falls back to `1` |
| `ThreadPoolBasicTest` | Single task, task enqueuing tasks before shutdown |
| `ThreadPoolDtorTest` | Drain on destruction, enqueue-after-shutdown throws, repeated and concurrent `shutdown()` |
| `ThreadPoolEdgeCasesTest` | Empty queue, empty task |
| `ThreadPoolConcurrencyTest` | Tasks run off the main thread, queued tasks wait and then run, many producer threads enqueuing concurrently |

## Known limitations

- An exception thrown by a task terminates the program. This will be handled through `std::future`.
- Enqueuing a task from inside another task while the pool is shutting down is not supported yet: `enqueue_task` throws, and since exceptions escaping a task terminate the program, this ends the process.- Tasks cannot return values yet.

## Roadmap

- [ ] `std::future` support: return results and exceptions from tasks
- [ ] Logging
- [ ] Define the behavior of enqueuing from a task during shutdown

## License

MIT
