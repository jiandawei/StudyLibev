// ==============================================================================
// Michael & Scott 无锁 MPMC 链表队列 — 性能基准
//
// 程序功能：
//   对比三种 MPMC 队列实现在多生产者/多消费者场景下的吞吐量。
//
// 被测队列：
//   1. LockFreeMPMCQueue (环形数组 + 版本戳, 无锁, 有界)
//   2. LockFreeMSQueue   (链表 + CAS, 无锁, 无界)
//   3. std::queue + std::mutex (有锁, 有界, 性能参考基线)
//
// 测试方法：
//   每组配置启动 P 个生产者和 C 个消费者线程。
//   所有线程通过 Barrier 同步同时开始。
//   生产者写入 items_per_prod 个 uint64_t，消费者读取直到消费总数达标。
//   记录耗时并计算吞吐量 (ops/s)。
//
// 入队策略抽象：
//   有界队列 (LockFreeMPMCQueue / Mutex) 需要 while(!enqueue(...)) 自旋，
//   因为队列可能临时满。无界队列 (LockFreeMSQueue) 的 enqueue 永远成功，
//   直接调用即可。通过 bounded_enqueue / unbounded_enqueue 两个 lambda
//   统一接口，test_mpmc 和 run_bench 无需关心底层差异。
// ==============================================================================

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
#include "lockfree_ms_queue.hpp"

using namespace std;
using namespace chrono;

// ==============================================================================
// 辅助工具
// ==============================================================================

/// @brief 线程屏障 (Barrier)
///
/// 用于保证所有线程尽可能同时开始工作，消除线程创建/调度延迟对测量结果的影响。
///
/// 用法：
///   Barrier bar(N);     // N 个线程参与
///   bar.arrive_and_wait(); // 阻塞直到 N 个线程都到达此点
///
/// 实现：基于 mutex + condition_variable + 计数器。
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
// 多线程 MPMC 正确性验证（泛型）
// ==============================================================================

/// @brief 多线程 MPMC 正确性验证
///
/// @tparam Q      队列类型（需要提供 dequeue 方法）
/// @tparam EnqFn  入队函数类型（接收 Q& 和值，返回 bool 表示成功）
/// @param q       队列引用
/// @param enq     入队策略（bounded_enqueue 或 unbounded_enqueue）
/// @param seen    元素是否被消费到的标记数组
/// @param P       生产者线程数
/// @param C       消费者线程数
/// @param N       每个生产者产生的元素数
template<typename Q, typename EnqFn>
void test_mpmc(Q& q, EnqFn&& enq, vector<uint64_t>& seen,
               size_t P, size_t C, size_t N)
{
    size_t TOTAL = P * N;
    Barrier bar(P + C);

    // 生产者：每个线程产生 N 个元素，值范围为 [p*N, (p+1)*N)
    vector<thread> prod;
    for (size_t p = 0; p < P; p++)
        prod.emplace_back([&, p] {
            bar.arrive_and_wait();
            for (size_t i = 0; i < N; )
                if (enq(q, p * N + i)) i++;
        });

    // 消费者：持续读取直到消费总数达到 TOTAL
    vector<thread> cons;
    atomic<size_t> done{0};
    for (size_t c = 0; c < C; c++)
        cons.emplace_back([&] {
            bar.arrive_and_wait();
            uint64_t v;
            while (done.load(memory_order_acquire) < TOTAL)
                if (q.dequeue(v)) {
                    seen[v] = 1;  // 标记该值被消费过
                    done.fetch_add(1, memory_order_release);
                }
        });

    for (auto& t : prod) t.join();
    for (auto& t : cons) t.join();
}

// ==============================================================================
// 通用性能基准函数
// ==============================================================================

struct BenchResult {
    string name;   // 队列名称
    double tps;    // 每秒吞吐量
    long ms;       // 总耗时 (毫秒)
};

/// @brief 运行单组 P/C 配置的性能测试
///
/// @tparam Q      队列类型
/// @tparam EnqFn  入队策略
/// @param q       队列引用
/// @param enq     入队函数
/// @param name    日志显示的队列名称
/// @param P       生产者数
/// @param C       消费者数
/// @param items_per_prod 每个生产者产生的元素数
template<typename Q, typename EnqFn>
static BenchResult run_bench(Q& q, EnqFn&& enq, const string& name,
                              size_t P, size_t C, uint64_t items_per_prod)
{
    Barrier bar(P + C);
    uint64_t total = P * items_per_prod;
    atomic<uint64_t> consumed{0};

    auto start = high_resolution_clock::now();

    // 启动生产者线程
    vector<thread> prod;
    for (size_t i = 0; i < P; i++)
        prod.emplace_back([&] {
            bar.arrive_and_wait();
            for (uint64_t n = 0; n < items_per_prod; )
                if (enq(q, n)) n++;
        });

    // 启动消费者线程
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

// ==============================================================================
// 入队策略 lambda
//
// 有界队列 (LockFreeMPMCQueue, Mutex) 的 enqueue 返回 bool：
//   false 表示队列临时满需要重试, 用 while 自旋直到成功
//
// 无界队列 (LockFreeMSQueue) 的 enqueue 返回 void：
//   永远成功，直接调用即可
//
// 使用方式：传递给 test_mpmc() 或 run_bench() 的第三个参数
// ==============================================================================

/// 有界队列入队策略：自旋直到成功
auto bounded_enqueue = [](auto& q, auto v) -> bool {
    return q.enqueue(v);
};

/// 无界队列入队策略：直接入队（总是成功）
auto unbounded_enqueue = [](auto& q, auto v) -> bool {
    q.enqueue(v);
    return true;
};

// ==============================================================================
// 队列包装器
// ==============================================================================

/// @brief LockFreeMPMCQueue (环形) 包装
///
/// 封装 LockFreeMPMCQueue<uint64_t> 使其满足 run_bench 接口要求。
struct LockFreeWrap {
    LockFreeMPMCQueue<uint64_t> q;
    LockFreeWrap(size_t cap) : q(cap) {}
    bool enqueue(uint64_t v) { return q.enqueue(v); }
    bool dequeue(uint64_t& v) { return q.dequeue(v); }
};

/// @brief LockFreeMSQueue (链表) 包装
struct MSQueueWrap {
    LockFreeMSQueue<uint64_t> q;
    void enqueue(uint64_t v) { q.enqueue(v); }
    bool dequeue(uint64_t& v) { return q.dequeue(v); }
};

/// @brief std::mutex + std::queue 包装（有锁基准）
struct MutexQueueWrap {
    mutex mtx;
    queue<uint64_t> q;
    size_t cap;
    explicit MutexQueueWrap(size_t c) : cap(c) {}
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
// main
// ==============================================================================

int main()
{
    cout << "===================================================\n"
            " 无锁 MPMC 队列性能基准\n"
            " 对比: LockFree(环形) vs MS(链表) vs Mutex(有锁)\n"
            "===================================================\n\n";

    // ============================================================
    // 1. 正确性验证
    // ============================================================

    cout << "--- 正确性验证 ---\n";

    // 1a) LockFreeMPMCQueue 单线程基本操作
    {
        LockFreeWrap lf(1024);
        assert(lf.enqueue(10));
        assert(lf.enqueue(20));
        uint64_t v;
        assert(lf.dequeue(v) && v == 10);
        assert(lf.dequeue(v) && v == 20);
        assert(!lf.dequeue(v));  // 队列空了
        cout << "  [PASS] LockFree(环形) 单线程\n";
    }

    // 1b) LockFreeMSQueue 单线程基本操作
    {
        MSQueueWrap ms;
        ms.enqueue(10);
        ms.enqueue(20);
        ms.enqueue(30);
        uint64_t v;
        assert(ms.dequeue(v) && v == 10);
        assert(ms.dequeue(v) && v == 20);
        assert(ms.dequeue(v) && v == 30);
        assert(!ms.dequeue(v));  // 队列空了
        cout << "  [PASS] MS(链表) 单线程\n";
    }

    // 1c) 多线程 MPMC 正确性
    //     4 个生产者各写 25000 个元素，4 个消费者并发读取
    //     验证所有值都被消费到且每个值恰好一次
    constexpr size_t P_TEST = 4, C_TEST = 4, N_TEST = 25000;
    constexpr size_t TOTAL_TEST = P_TEST * N_TEST;

    {
        LockFreeWrap lf(65536);
        vector<uint64_t> seen(TOTAL_TEST, 0);
        test_mpmc(lf, bounded_enqueue, seen, P_TEST, C_TEST, N_TEST);
        assert(accumulate(seen.begin(), seen.end(), 0ULL) == TOTAL_TEST);
        cout << "  [PASS] LockFree(环形) 多线程 MPMC\n";
    }

    {
        MSQueueWrap ms;
        vector<uint64_t> seen(TOTAL_TEST, 0);
        test_mpmc(ms, unbounded_enqueue, seen, P_TEST, C_TEST, N_TEST);
        assert(accumulate(seen.begin(), seen.end(), 0ULL) == TOTAL_TEST);
        cout << "  [PASS] MS(链表) 多线程 MPMC\n";
    }

    // ============================================================
    // 2. 性能基准
    // ============================================================

    constexpr size_t CAP = 1 << 20;   // 有界队列容量 1M
    constexpr uint64_t PER = 500000;  // 每线程生产 50 万项

    // 测试多种生产者/消费者比例，覆盖不同竞争程度
    struct Config { size_t P, C; };
    Config configs[] = {{1,1}, {2,2}, {4,4}, {1,4}, {4,1}};

    cout << "\n--- 性能结果 ---\n";
    for (auto [P, C] : configs) {
        cout << "\n--- P=" << P << " C=" << C << " ---\n";

        {
            LockFreeWrap lf(CAP);
            run_bench(lf, bounded_enqueue,   "LockFree(环形)", P, C, PER);
        }
        {
            MSQueueWrap ms;
            run_bench(ms, unbounded_enqueue, "MS(链表)     ", P, C, PER);
        }
        {
            MutexQueueWrap mq(CAP);
            run_bench(mq, bounded_enqueue,   "Mutex        ", P, C, PER);
        }
    }
}
