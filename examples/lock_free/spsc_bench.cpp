// ==============================================================================
// LockFreeSPSCQueue — 单生产者单消费者无锁环形队列性能基准
//
// 程序功能：
//   对比 SPSC 无锁队列与 mutex 版有锁队列在 1P1C 场景下的吞吐量。
//
// 被测队列：
//   1. LockFreeSPSCQueue (环形无锁, SPSC)
//   2. std::queue + std::mutex (有锁基线)
//
// 测试逻辑：
//   1 个生产者和 1 个消费者通过 Barrier 同时启动。
//   生产者写入 PER 个 uint64_t，消费者全部读取。
// ==============================================================================

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <cassert>
#include <atomic>
#include <numeric>
#include "lockfree_spsc_queue.hpp"

using namespace std;
using namespace chrono;

// ==============================================================================
// 辅助工具
// ==============================================================================

/// 线程屏障：保证所有线程同时开始。
struct Barrier {
    mutex mtx;
    condition_variable cv;
    size_t wait_cnt{0};
    size_t total;
    explicit Barrier(size_t t) : total(t) {}
    void arrive_and_wait() {
        unique_lock lock(mtx);
        if (++wait_cnt == total) { cv.notify_all(); }
        else { cv.wait(lock, [this] { return wait_cnt == total; }); }
    }
};

// ==============================================================================
// 队列包装
// ==============================================================================

/// LockFreeSPSCQueue 包装
struct SPSCWrap {
    LockFreeSPSCQueue<uint64_t> q;
    explicit SPSCWrap(size_t cap) : q(cap) {}
    bool enqueue(uint64_t v) { return q.enqueue(v); }
    bool dequeue(uint64_t& v) { return q.dequeue(v); }
};

/// std::mutex + std::queue 包装
struct MutexQueue {
    mutex mtx;
    queue<uint64_t> q;
    size_t cap;
    explicit MutexQueue(size_t c) : cap(c) {}
    bool enqueue(uint64_t v) {
        lock_guard lock(mtx);
        if (q.size() >= cap) return false;
        q.push(v);
        return true;
    }
    bool dequeue(uint64_t& v) {
        lock_guard lock(mtx);
        if (q.empty()) return false;
        v = q.front();
        q.pop();
        return true;
    }
};

// ==============================================================================
// 正确性测试
// ==============================================================================

void test_single()
{
    SPSCWrap q(1024);
    assert(q.enqueue(10));
    assert(q.enqueue(20));
    uint64_t v;
    assert(q.dequeue(v) && v == 10);
    assert(q.dequeue(v) && v == 20);
    assert(!q.dequeue(v));
    cout << "  [PASS] LockFreeSPSC 单线程\n";
}

void test_1p1c()
{
    constexpr size_t N = 50000;

    SPSCWrap q(65536);
    vector<char> seen(N, 0);
    Barrier bar(2);

    thread prod([&] {
        bar.arrive_and_wait();
        for (size_t i = 0; i < N; i++)
            while (!q.enqueue(i));
    });

    thread cons([&] {
        bar.arrive_and_wait();
        uint64_t v;
        size_t done = 0;
        while (done < N)
            if (q.dequeue(v)) {
                seen[v] = 1;
                done++;
            }
    });

    prod.join();
    cons.join();

    size_t sum = accumulate(seen.begin(), seen.end(), 0ULL);
    assert(sum == N);
    cout << "  [PASS] LockFreeSPSC 1P1C N=" << N << "\n";
}

// ==============================================================================
// 性能基准
// ==============================================================================

template<typename Q>
static void run_bench(Q& q, const string& name, uint64_t items)
{
    Barrier bar(2);
    uint64_t total = items;
    atomic<uint64_t> consumed{0};

    auto start = high_resolution_clock::now();

    thread prod([&] {
        bar.arrive_and_wait();
        for (uint64_t n = 0; n < items; n++)
            while (!q.enqueue(n));
    });

    thread cons([&] {
        bar.arrive_and_wait();
        uint64_t v;
        while (consumed.load(memory_order_acquire) < total)
            if (q.dequeue(v))
                consumed.fetch_add(1, memory_order_release);
    });

    prod.join();
    cons.join();

    auto ms = duration_cast<milliseconds>(
        high_resolution_clock::now() - start).count();
    double tps = double(total) / ms * 1000.0;
    cout << "  " << name << ": " << total << " 项, "
         << ms << " ms, " << tps / 1e6 << " M ops/s\n";
}

// ==============================================================================
// main
// ==============================================================================

int main()
{
    cout << "==============================================\n"
            " LockFreeSPSC 性能基准\n"
            " 对比: SPSC 无锁 vs std::mutex\n"
            "==============================================\n\n";

    // 正确性
    test_single();
    test_1p1c();

    // 性能
    constexpr size_t CAP = 1 << 20;
    constexpr uint64_t PER = 50000000;

    cout << "\n--- 1P1C 性能 (各 " << PER << " 项) ---\n";
    {
        SPSCWrap q(CAP);
        run_bench(q, "LockFreeSPSC", PER);
    }
    {
        MutexQueue mq(CAP);
        run_bench(mq, "Mutex       ", PER);
    }
}
