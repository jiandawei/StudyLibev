#pragma once

// ==============================================================================
// LockFreeMSQueue — Michael & Scott 无锁 MPMC 链表队列 (1996)
//
// 论文: "Simple, Fast, and Practical Non-Blocking and Blocking Concurrent
//       Queue Algorithms" (Maged M. Michael, Michael L. Scott, PODC 1996)
//
// 数据结构：
//   - 单向链表，每个节点包含 T data + atomic<Node*> next
//   - head_ 永远指向一个哨兵(dummy)节点，head_->next 才是第一个有效数据节点
//   - tail_ 指向链表的最后一个节点（可能是哨兵也可能是数据节点）
//
// 为什么需要哨兵节点？
//   队列为空时 head_ == tail_ 指向同一个 dummy 节点，这样出队时总能通过
//   head_->next 判断是否空，无需对 nullptr 做特殊处理。
//
// 入队 (enqueue):
//   1. 分配新节点，构造数据，next = nullptr
//   2. 读取 tail_ 和 tail_->next
//   3. 如果 tail_->next == nullptr:
//        CAS(tail_->next, nullptr, new_node) — 尝试链接
//        成功后再尝试 CAS 推进 tail_ 到 new_node（允许失败，由其他线程代劳）
//     否则:
//        CAS(tail_, last, tail_->next) — 帮助其他线程推进 tail_
//
// 出队 (dequeue):
//   1. 读取 head_, tail_, head_->next
//   2. 如果 head_ == tail_:
//        - next == nullptr → 队列空
//        - next != nullptr → tail 滞后，帮助推进
//     否则 (有数据):
//        CAS(head_, first, next) — 将 head 从哨兵移到下一个节点
//        成功: 从 next->data 取出数据（此时我们独占 next 节点）
//
// 内存回收 (Memory Reclamation):
//   本实现不释放旧节点。因为一个线程在释放节点时，另一个线程可能正在读取
//   该节点的 next 指针 (use-after-free)。安全回收需要 Hazard Pointer、
//   RCU (Read-Copy-Update) 或 EBR (Epoch-Based Reclamation) 等机制，
//   不在本 Demo 范围内。
//
// 无界队列：链表动态增长，无容量上限。
// ==============================================================================

#include <atomic>
#include <utility>
#include <cstddef>

/// @brief Michael & Scott 经典无锁 MPMC 链表队列
/// @details 无界队列，支持多个生产者和消费者同时操作。
///          内部使用哨兵节点简化边界处理。
/// @tparam T 队列元素类型
template<typename T>
class LockFreeMSQueue
{
public:
    /// @brief 构造函数：创建空队列
    ///
    /// 初始化步骤：
    ///   1. 分配一个哨兵节点 (dummy)，此时它既是 head 也是 tail
    ///   2. dummy->next = nullptr
    ///   3. head_ = tail_ = dummy
    ///
    /// 这样队列在任何时刻都至少有一个节点，出队操作无需单独处理空链表情况。
    LockFreeMSQueue()
    {
        Node* dummy = static_cast<Node*>(::operator new(sizeof(Node)));
        dummy->next.store(nullptr, std::memory_order_relaxed);
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    /// @brief 析构函数
    ///
    /// 逐个出队所有剩余元素，最后一个哨兵节点留在链上不释放。
    /// （因为无 Hazard Pointer，释放可能导致其他线程 use-after-free）
    ~LockFreeMSQueue()
    {
        T dummy;
        while (dequeue(dummy)) {}
        // 简易实现：不回收哨兵节点，生产环境需配合 Hazard Pointer
    }

    // 禁止拷贝
    LockFreeMSQueue(const LockFreeMSQueue&) = delete;
    LockFreeMSQueue& operator=(const LockFreeMSQueue&) = delete;

    /// @brief 入队（拷贝语义）
    /// @param item 左值引用
    void enqueue(const T& item)
    {
        do_enqueue(item);
    }

    /// @brief 入队（移动语义）
    /// @param item 右值引用
    void enqueue(T&& item)
    {
        do_enqueue(std::move(item));
    }

    /// @brief 出队操作
    ///
    /// 关键设计：先 CAS 推进 head_，成功后再读取数据。
    /// 这保证同一节点只被一个消费者"拥有"，不会出现多个消费者
    /// 同时 std::move 同一节点数据导致数据丢失。
    ///
    /// @param out 接收出队元素的引用
    /// @return 成功 true，空队列 false
    bool dequeue(T& out)
    {
        while (true)
        {
            // 1. 快照当前队列状态
            Node* first = head_.load(std::memory_order_acquire);  // 哨兵
            Node* last  = tail_.load(std::memory_order_acquire);  // 尾节点
            Node* next  = first->next.load(std::memory_order_acquire); // 第一个数据节点

            // 2. 一致性检查：如果 head_ 在我们读取期间被修改了，重试
            //    否则可能读到属于另一个消费者已经处理的节点
            if (first != head_.load(std::memory_order_relaxed))
                continue;

            // 3. head_ == tail_ 表示尾节点就是哨兵节点（链表为空或 tail 滞后）
            if (first == last)
            {
                // next == nullptr: 哨兵之后没有节点 → 队列空
                if (next == nullptr)
                    return false;
                // next != nullptr: 有节点但 tail_ 还没指向它，
                // 帮助推进 tail_（"helping" 机制，MS 算法的关键设计）
                tail_.compare_exchange_weak(last, next,
                    std::memory_order_release, std::memory_order_relaxed);
                // 循环回去再次检查
            }
            else
            {
                // 4. first != last: 链表中有至少一个数据节点
                //    next 就是第一个数据节点
                //    尝试 CAS head_ from first → next，
                //    原子地将哨兵从 first 推进到 next
                if (head_.compare_exchange_weak(first, next,
                    std::memory_order_release, std::memory_order_relaxed))
                {
                    // CAS 成功！此线程独占 next 节点。
                    // 此时 next 已被移出队列（next 变成了新哨兵），
                    // 其他消费者不会再读到 next，所以可以安全地 move data
                    out = std::move(next->data);
                    // first（旧哨兵）不再需要，但简易实现不释放它
                    // （释放可能导致正在读取 first->next 的另一线程崩溃）
                    return true;
                }
                // CAS 失败：其他消费者抢先一步推进了 head_，重试
            }
        }
    }

private:
    /// @brief 通用入队模板
    ///
    /// 设计要点（数据先构造再入链）：
    ///   先把数据构造到新节点上，再将节点链接到链表末尾。
    ///   顺序不可颠倒——如果先链接再构造，消费者可能在数据构造完成
    ///   之前就读到新节点并尝试 std::move 数据，导致数据损坏。
    ///
    /// 帮助机制 (helping)：
    ///   如果一个线程发现 tail_->next != nullptr（说明另一个线程已链接
    ///   了新节点但尚未推进 tail_），它会帮忙推进 tail_。
    ///   这是无锁算法中常见的协作模式，确保 progress。
    ///
    /// @tparam U 元素类型（左值或右值引用）
    /// @param item 元素
    template<typename U>
    void do_enqueue(U&& item)
    {
        // 1. 分配节点 + 构造数据（先构造！）
        Node* node = static_cast<Node*>(::operator new(sizeof(Node)));
        new (&node->data) T(std::forward<U>(item));
        node->next.store(nullptr, std::memory_order_relaxed);

        while (true)
        {
            // 2. 快照 tail 状态
            Node* last = tail_.load(std::memory_order_acquire);
            Node* next = last->next.load(std::memory_order_acquire);

            // 一致性检查：tail_ 在我们读取期间不能改变
            if (last != tail_.load(std::memory_order_relaxed))
                continue;

            if (next == nullptr)
            {
                // 3. last 确实是尾节点，尝试将新节点链接到 last->next
                //    compare_exchange_weak(next, node):
                //    期望 last->next == nullptr, 如果成立就设为 node
                //    否则 next 会被更新为 last->next 的实际值
                if (last->next.compare_exchange_weak(next, node,
                    std::memory_order_release, std::memory_order_relaxed))
                {
                    // 4. 链接成功！尝试推进 tail_ 到新节点
                    //    这里用 compare_exchange_strong 而非 weak：
                    //    因为不希望在 spurious wakeup 下意外失败
                    //    即使 CAS 失败也不要紧——其他线程会帮忙推进
                    tail_.compare_exchange_strong(last, node,
                        std::memory_order_release, std::memory_order_relaxed);
                    return;
                }
            }
            else
            {
                // tail 滞后于实际链表尾，帮助推进
                // 这是 MS 算法的关键：线程之间互相帮忙完成对方未完成的操作
                tail_.compare_exchange_weak(last, next,
                    std::memory_order_release, std::memory_order_relaxed);
            }
        }
    }

    /// @brief 链表节点
    struct Node
    {
        T data;                     // 节点承载的数据
        std::atomic<Node*> next{nullptr}; // 指向下一个节点，CAS 操作的目标
    };

    /// head_：指向哨兵节点
    /// head_->next 是第一个有效数据节点（或 nullptr 表示队列空）
    /// 出队时 CAS head_ 来推进
    alignas(64) std::atomic<Node*> head_;

    /// tail_：指向链表的最后一个节点
    /// 入队时在 tail_ 后面追加新节点，并推进 tail_
    /// alignas(64) 隔离缓存行，避免与 head_ 伪共享
    alignas(64) std::atomic<Node*> tail_;
};
