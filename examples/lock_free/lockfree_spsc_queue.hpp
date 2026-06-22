#pragma once

#include <atomic>
#include <cstddef>
#include <utility>
#include <stdexcept>

// ==============================================================================
// LockFreeSPSCQueue — 单生产者单消费者无锁环形队列 (Lamport 风格)
//
// 适用范围：严格 1 写 1 读，不允许 >= 2 个线程同时写入或读取。
//
// 为什么不需要 CAS？
//   read_ 只被消费者写, write_ 只被生产者写。
//   没有多线程同时竞争同一个位置，所以无需 CAS。
//   唯一需要同步的是：消费者要看到生产者写入的数据，生产者要看到消费者的读取进度。
//   这通过 acquire/release 内存序保证。
//
// 内存序说明：
//   - 生产者写入数据后用 release 更新 write_，消费者用 acquire 读取 write_
//     保证消费者看到写入的数据（happens-before）
//   - 消费者读取数据后用 release 更新 read_，生产者用 acquire 读取 read_
//     保证生产者看到槽位已被消费（可以安全覆盖）
//
// vs MPMC 环形队列：
//   SPSC 因为不需要 CAS，读写路径更短，性能更高。
//   但也需要 2 的幂次容量，利用 mask 做快速取模。
//
// 伪共享隔离：
//   write_ (只被生产者写) 与 read_ (只被消费者读) 各自独占缓存行。
//   槽位数组本身已对齐以避免相邻槽位互相干扰。
// ==============================================================================

/// @brief 单生产者单消费者无锁环形队列
///
/// 经典 Lamport 无锁 SPSC 环形队列。
/// 要求容量必须是 2 的幂次，利用位运算做快速取模。
///
/// @tparam T 元素类型
template<typename T>
class LockFreeSPSCQueue
{
public:
    /// @brief 构造函数
    /// @param capacity 容量，必须是 2 的幂次
    explicit LockFreeSPSCQueue(size_t capacity)
    {
        if ((capacity & (capacity - 1)) != 0)
            throw std::invalid_argument{"Capacity must be power of two"};
        capacity_ = capacity;
        mask_ = capacity - 1;
        buffer_ = new Slot[capacity_];
    }

    ~LockFreeSPSCQueue()
    {
        size_t r = read_.load(std::memory_order_relaxed);
        size_t w = write_.load(std::memory_order_relaxed);
        // 析构未消费的元素
        for (size_t i = r; i < w; ++i)
            std::destroy_at(&buffer_[i & mask_].data);
        delete[] buffer_;
    }

    LockFreeSPSCQueue(const LockFreeSPSCQueue&) = delete;
    LockFreeSPSCQueue& operator=(const LockFreeSPSCQueue&) = delete;

    /// @brief 入队（仅生产者线程调用）
    /// @return 成功 true，队列满 false
    bool enqueue(const T& item)
    {
        size_t w = write_.load(std::memory_order_relaxed);
        size_t r = read_.load(std::memory_order_acquire);
        // 队列满: 可用的只剩下 1 个都不留（full = capacity - 1）
        // 因为如果 write_ + capacity == read_，消费者已读走但 write_ 还没绕回来？
        // 实际上: read_ 是已读位置, write_ 是下一写入位置
        // 已用 = write_ - read_, 可用 = capacity_ - (write_ - read_)
        // 留 1 个空位区分满和空: 满 = write_ + 1 == read_ (模 capacity)
        if (w - r >= capacity_)
            return false;

        Slot& slot = buffer_[w & mask_];
        new (&slot.data) T(item);
        // release: 保证 data 写入在 write_ 更新前对其他线程可见
        write_.store(w + 1, std::memory_order_release);
        return true;
    }

    /// @brief 入队（移动语义）
    bool enqueue(T&& item)
    {
        size_t w = write_.load(std::memory_order_relaxed);
        size_t r = read_.load(std::memory_order_acquire);
        if (w - r >= capacity_)
            return false;

        Slot& slot = buffer_[w & mask_];
        new (&slot.data) T(std::move(item));
        write_.store(w + 1, std::memory_order_release);
        return true;
    }

    /// @brief 出队（仅消费者线程调用）
    /// @param out 接收元素
    /// @return 成功 true，队列空 false
    bool dequeue(T& out)
    {
        size_t r = read_.load(std::memory_order_relaxed);
        size_t w = write_.load(std::memory_order_acquire);
        // 队列空
        if (r == w)
            return false;

        Slot& slot = buffer_[r & mask_];
        out = std::move(slot.data);
        std::destroy_at(&slot.data);
        // release: 保证读取 data 后再更新 read_，让生产者看到消费进度
        read_.store(r + 1, std::memory_order_release);
        return true;
    }

    size_t capacity() const noexcept { return capacity_; }

private:
    struct Slot
    {
        alignas(64) T data;
    };

    Slot* buffer_{nullptr};
    size_t capacity_{0};
    size_t mask_{0};

    // write_ 只被生产者写、只被消费者读
    alignas(64) std::atomic<size_t> write_{0};
    // read_ 只被消费者写、只被生产者读
    alignas(64) std::atomic<size_t> read_{0};
};
