// ============================================================
// LockFreeMPMCQueue — 无锁多生产者多消费者环形队列性能基准
//
// 对比两种队列实现：
//   1. LockFreeWrap — 基于 LockFreeMPMCQueue (无锁)
//   2. MutexQueue   — std::queue + std::mutex (有锁基准)
//
// 测试多组 P（生产者数）/ C（消费者数）组合，输出吞吐量。
// ============================================================

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <cassert>
#include <atomic>
#include <numeric>
#include "lockfree_mpmc_queue.hpp"

using namespace std;
using namespace chrono;

// ========== 正确性测试 ==========

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

void test_single()
{
    LockFreeMPMCQueue<int> q{1024};
    assert(q.enqueue(10));
    assert(q.enqueue(20));
    int v;
    assert(q.dequeue(v) && v == 10);
    assert(q.dequeue(v) && v == 20);
    assert(!q.dequeue(v));
    cout << "[PASS] 单线程\n";
}

void test_mpmc()
{
    constexpr size_t P = 4, C = 4, N = 25000, TOTAL = P * N;
    LockFreeMPMCQueue<size_t> q{65536};
    vector<char> seen(TOTAL, 0);
    Barrier bar(P + C);

    vector<thread> prod;
    for (size_t p = 0; p < P; p++)
        prod.emplace_back([&, p] {
            bar.arrive_and_wait();
            for (size_t i = 0; i < N; i++)
                while (!q.enqueue(p * N + i));
        });

    vector<thread> cons;
    atomic<size_t> done{0};
    for (size_t c = 0; c < C; c++)
        cons.emplace_back([&] {
            bar.arrive_and_wait();
            size_t v;
            while (done.load(memory_order_acquire) < TOTAL)
                if (q.dequeue(v)) {
                    seen[v] = 1;
                    done.fetch_add(1, memory_order_release);
                }
        });

    for (auto& t : prod) t.join();
    for (auto& t : cons) t.join();

    assert(accumulate(seen.begin(), seen.end(), 0ULL) == TOTAL);
    cout << "[PASS] 多线程 MPMC\n";
}

// ========== 性能测试 & Bench ==========

struct SpscQueue {
    virtual bool enqueue(uint64_t) = 0;
    virtual bool dequeue(uint64_t&) = 0;
    virtual ~SpscQueue() = default;
};

struct LockFreeWrap : SpscQueue {
    LockFreeMPMCQueue<uint64_t> q;
    LockFreeWrap(size_t cap) : q(cap) {}
    bool enqueue(uint64_t v) override { return q.enqueue(v); }
    bool dequeue(uint64_t& v) override { return q.dequeue(v); }
};

struct MutexQueue : SpscQueue {
    mutex mtx;
    queue<uint64_t> q;
    size_t cap;
    explicit MutexQueue(size_t c) : cap(c) {}
    bool enqueue(uint64_t v) override {
        lock_guard lock(mtx);
        if (q.size() >= cap) return false;
        q.push(v);
        return true;
    }
    bool dequeue(uint64_t& v) override {
        lock_guard lock(mtx);
        if (q.empty()) return false;
        v = q.front();
        q.pop();
        return true;
    }
};

struct BenchResult {
    string name;
    double tps;
    long ms;
};

static BenchResult run_bench(SpscQueue& q, const string& name,
                             size_t P, size_t C, uint64_t items_per_prod)
{
    Barrier bar(P + C);
    uint64_t total = P * items_per_prod;
    atomic<uint64_t> consumed{0};

    auto start = high_resolution_clock::now();

    vector<thread> prod;
    for (size_t i = 0; i < P; i++)
        prod.emplace_back([&] {
            bar.arrive_and_wait();
            for (uint64_t n = 0; n < items_per_prod; n++)
                while (!q.enqueue(n));
        });

    vector<thread> cons;
    for (size_t i = 0; i < C; i++)
        cons.emplace_back([&] {
            bar.arrive_and_wait();
            uint64_t v;
            while (consumed.load(memory_order_acquire) < total)
                if (q.dequeue(v))
                    consumed.fetch_add(1, memory_order_release);
        });

    for (auto& t : prod) t.join();
    for (auto& t : cons) t.join();

    auto ms = duration_cast<milliseconds>(
        high_resolution_clock::now() - start).count();
    double tps = double(total) / ms * 1000.0;

    cout << "  " << name << ": " << total << " 项, "
         << ms << " ms, " << tps / 1e6 << " M ops/s\n";
    return {name, tps, ms};
}

int main()
{
    cout << "============================================\n"
            " LockFreeMPMCQueue 性能基准\n"
            " 对比: LockFree vs std::mutex + std::queue\n"
            "============================================\n\n";

    // === 1. 正确性验证 ===
    test_single();
    test_mpmc();

    // === 2. 性能基准 ===
    constexpr size_t CAP = 1 << 20;
    constexpr uint64_t PER = 500000;

    struct Config { size_t P, C; };
    Config configs[] = {{1,1}, {2,2}, {4,4}, {1,4}, {1,8}, {4,1}, {8, 8}};

    cout << "\n--- 性能结果 ---\n";
    for (auto [P, C] : configs) {
        cout << "\n--- P=" << P << " C=" << C << " ---\n";
        {
            LockFreeWrap lf(CAP);
            run_bench(lf, "lock-free", P, C, PER);
        }
        {
            MutexQueue mq(CAP);
            run_bench(mq, "mutex   ", P, C, PER);
        }
    }
}
