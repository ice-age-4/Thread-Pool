// Example: summing a large array in parallel with ThreadPool,
// and printing which thread handled each chunk.

#include <cstdint>
#include <iostream>
#include <mutex>
#include <numeric>
#include <sstream>
#include <thread>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include "ThreadPool.hpp"

constexpr std::size_t kChunks = 8;
constexpr std::size_t kChunkSize = 1'000'000;
constexpr std::size_t kTotalSize = kChunks * kChunkSize;

int main() {

    // data = 1, 2, 3, ..., 8,000,000
    std::vector<std::uint64_t> data(kTotalSize);
    std::iota(data.begin(), data.end(), std::uint64_t{1});

    // Each task writes only to its own slot, so no lock is needed here.
    std::vector<std::uint64_t> partial_sums(kChunks, 0);
    std::mutex cout_mutex;  // keeps the output of different threads from mixing

    ThreadPool pool;
    std::cout << "Pool size: " << pool.get_num_threads() << " threads\n\n";

    for (std::size_t i = 0; i < kChunks; ++i) {
        pool.enqueue_task([i, &data, &partial_sums, &cout_mutex] {
            auto begin = data.begin() + i * kChunkSize;
            auto end = begin + kChunkSize;
            partial_sums[i] = std::accumulate(begin, end, std::uint64_t{0});

            std::ostringstream line;
            line << "Chunk " << i << " -> C++ thread id: " << std::this_thread::get_id();
#ifdef _WIN32
            line << ", Windows thread id: " << GetCurrentThreadId();
#endif
            line << ", partial sum: " << partial_sums[i] << '\n';

            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << line.str();
        });
    }

    // Wait for all tasks to finish. After shutdown() returns, reading partial_sums is safe.
    pool.shutdown();

    const std::uint64_t total = std::accumulate(partial_sums.begin(), partial_sums.end(), std::uint64_t{0});
    const std::uint64_t expected = static_cast<std::uint64_t>(kTotalSize) * (kTotalSize + 1) / 2;

    std::cout << "\nTotal:    " << total << '\n'
              << "Expected: " << expected << '\n'
              << (total == expected ? "OK" : "MISMATCH") << '\n';
    return 0;
}
