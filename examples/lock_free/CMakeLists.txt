cmake_minimum_required(VERSION 3.20)
project(LockFreeQueueDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 关闭调试优化，性能测试用Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-O3)
endif()

add_executable(lockfree_demo bench.cpp lockfree_mpmc_queue.hpp)

add_executable(ms_demo ms_bench.cpp lockfree_mpmc_queue.hpp lockfree_ms_queue.hpp)

add_executable(spsc_demo spsc_bench.cpp lockfree_spsc_queue.hpp)