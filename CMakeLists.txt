cmake_minimum_required(VERSION 3.20)
project(ThreadPool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- make the thread pool library ---
find_package(Threads REQUIRED)
add_library(thread_pool STATIC src/ThreadPool.cpp)
target_include_directories(thread_pool PUBLIC include)
target_link_libraries(thread_pool PUBLIC Threads::Threads)

# --- main program ---
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE thread_pool)

# --- tests ---
include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(thread_pool_tests tests/thread_pool_tests.cpp)
target_link_libraries(thread_pool_tests PRIVATE thread_pool GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(thread_pool_tests)