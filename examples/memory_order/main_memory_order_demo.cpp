#include <iostream>
#include <atomic>
#include <thread>
#include <cassert>

using namespace std;

/**
 * 1. relaxed: 保证原子性但无顺序约束
 *    t2 可能看到 y=1 但 x=0，证明 CPU/编译器重排
 */
void demo_relaxed()
{
    cerr << "===== 1. relaxed: 无顺序保证 =====\n";
    atomic<int> x{0}, y{0};
    bool seen = false;

    for (int i = 0; i < 100000; i++)
    {
        x = 0; y = 0;
        thread t1([&]() {
            x.store(1, memory_order_relaxed);
            y.store(1, memory_order_relaxed);
        });
        thread t2([&]() {
            while (y.load(memory_order_relaxed) == 0);
            if (x.load(memory_order_relaxed) == 0)
                seen = true;
        });
        t1.join(); t2.join();
        if (seen) break;
    }
    if (seen)
        cerr << ">> 检测到重排: y=1 但 x=0\n";
    else
        cerr << ">> 100000 次均未触发（随机性）\n";
}

/**
 * 2. release + acquire: 经典生产者-消费者
 *    release: 之前所有写操作对 acquire 后的代码可见
 *    acquire: 之后所有读操作能看到 release 前的写入
 */
void demo_release_acquire()
{
    cerr << "===== 2. release + acquire 同步 =====\n";
    int data = 0;
    atomic<bool> ready{false};

    thread prod([&]() {
        data = 100;
        ready.store(true, memory_order_release);
    });
    thread cons([&]() {
        while (!ready.load(memory_order_acquire));
        assert(data == 100);
        cerr << ">> data = " << data << "\n";
    });
    prod.join(); cons.join();
}

/**
 * 3. seq_cst: 全局顺序一致性（默认内存序）
 *    StoreLoad 场景: 禁止 x=0 && y=0
 */
void demo_seq_cst()
{
    cerr << "===== 3. seq_cst 全局一致性 =====\n";
    bool bad = false;

    for (int i = 0; i < 100000; i++)
    {
        atomic<int> a{0}, b{0};
        int x = 0, y = 0;

        thread t1([&]() {
            a.store(1, memory_order_seq_cst);
            x = b.load(memory_order_seq_cst);
        });
        thread t2([&]() {
            b.store(1, memory_order_seq_cst);
            y = a.load(memory_order_seq_cst);
        });
        t1.join(); t2.join();
        if (x == 0 && y == 0) { bad = true; break; }
    }
    if (bad)
        cerr << ">> seq_cst 未阻止全局乱序（异常）\n";
    else
        cerr << ">> seq_cst 禁止 x=0&&y=0\n";
}

/**
 * 4. acq_rel: 双向屏障，用于 RMW 操作
 *    fetch_add 同时需要 acquire（读旧值）和 release（写新值）
 */
void demo_acq_rel()
{
    cerr << "===== 4. acq_rel 双向屏障 =====\n";
    atomic<int> cnt{0};
    int r1 = 0, r2 = 0;

    thread t1([&]() { r1 = cnt.fetch_add(1, memory_order_acq_rel); });
    thread t2([&]() { r2 = cnt.fetch_add(1, memory_order_acq_rel); });
    t1.join(); t2.join();

    cerr << ">> r1=" << r1 << " r2=" << r2 << " cnt=" << cnt.load() << "\n";
    assert((r1 == 0 && r2 == 1) || (r1 == 1 && r2 == 0));
}

/**
 * 5. 多生产者 + release/acquire: 消息传递
 *    每个生产者各自发布数据，消费者按序读取
 */
void demo_multi_producer()
{
    cerr << "===== 5. 多生产者消息传递 =====\n";
    atomic<int> flag{0};
    int msg1 = 0, msg2 = 0;

    thread p1([&]() { msg1 = 111; flag.store(1, memory_order_release); });
    thread p2([&]() { msg2 = 222; flag.store(2, memory_order_release); });
    thread cons([&]() {
        int v = flag.load(memory_order_acquire);
        if (v == 1) { assert(msg1 == 111); cerr << ">> msg1=" << msg1 << "\n"; }
        if (v == 2) { assert(msg2 == 222); cerr << ">> msg2=" << msg2 << "\n"; }
    });
    p1.join(); p2.join(); cons.join();
}

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    demo_relaxed();
    demo_release_acquire();
    demo_seq_cst();
    demo_acq_rel();
    demo_multi_producer();

    cerr << "\n=== 全部完成 ===\n";
    return 0;
}
