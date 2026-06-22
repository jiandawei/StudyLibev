#pragma once

#include <atomic>
#include <span>
#include <utility>
#include <cstddef>
#include <stdexcept>

// ==============================================================================
// LockFreeMPMCQueue — 无锁多生产者多消费者环形队列
//
// 核心思想（Dmitry Vyukov 经典设计）：
//   环形数组 + 单调递增版本戳 (stamp)。每个槽位有一个 stamp，值与 position
//   挂钩。线程通过 CAS 原子地抢占 position，利用 stamp 奇偶性区分槽位状态。
//
//   关键约束：容量必须是2的幂次，利用 mask = capacity-1 做快速取模。
//
// 版本戳协议：
//   每个槽位 i 的初始 stamp = i（偶数为空，归生产者；奇数为满，归消费者）。
//   - 生产者成功写入后，stamp = 原 position + 1（奇偶翻转，通知消费者）
//   - 消费者成功读取后，stamp = 原 position + capacity（奇偶回到偶数，通知下一轮生产者）
//
// 这样做的好处：同一槽位在不同轮次中的奇偶性保持一致，
// 即使 position 环绕多圈也不会与旧数据混淆，天然免疫 ABA 问题。
//
// 伪共享隔离：
//   head_ / tail_ / Slot.stamp / Slot.data 各占独立缓存行 (alignas 64)，
//   避免多核 CPU 下相邻变量因缓存一致性协议带来的性能衰减。
// ==============================================================================

/// @brief C++20 无锁多生产者多消费者(MPMC)环形队列
/// @details 基于Dmitry Vyukov经典的基于Stamp标记的无锁环形队列实现
///          要求队列容量必须为2的幂次，利用位运算快速取模计算槽位索引
///          通过奇偶标记区分槽位状态，避免ABA问题，多线程下无需互斥锁
/// @tparam T 队列存储的元素类型，支持拷贝构造、移动构造、析构
template<typename T>
class LockFreeMPMCQueue
{
public:
    /// @brief 构造函数，初始化无锁环形队列
    /// @param capacity 队列最大容量，必须是2的整数次幂(如2/4/8/16...)
    /// @throw std::invalid_argument 当传入容量不是2的幂次时抛出异常
    explicit LockFreeMPMCQueue(size_t capacity)
    {
        // 位运算判断是否为2的幂次：只有一个二进制位为1
        // 例如 capacity=8 (1000₂): (8 & 7) = 0, 有效
        //      capacity=9 (1001₂): (9 & 8) = 8 ≠ 0, 无效
        if ((capacity & (capacity - 1)) != 0)
        {
            throw std::invalid_argument{"Capacity must be power of two"};
        }
        capacity_ = capacity;
        // 掩码，用于快速取模计算索引：index = pos & mask_，替代%运算提升性能
        // 例如 capacity=8, mask_=7 (111₂), pos=9 → 9 & 7 = 1 (相当于 9 % 8)
        mask_ = capacity - 1;
        // 动态分配槽位数组
        buffer_ = new Slot[capacity_];
        // 初始化每个槽位的版本戳stamp，初始值为当前槽位索引
        // 例如 slot[0].stamp = 0, slot[1].stamp = 1, ..., slot[capacity-1].stamp = capacity-1
        // 偶数初始 = 空 (生产者持有), 后续每轮循环奇偶交替
        for (size_t i = 0; i < capacity_; ++i)
        {
            buffer_[i].stamp.store(i, std::memory_order_relaxed);
        }
    }

    /// @brief 析构函数，安全销毁队列中已构造的元素并释放堆内存
    /// @details 遍历所有槽位，通过stamp奇偶位判断元素是否有效，有效则调用元素析构函数
    ~LockFreeMPMCQueue()
    {
        for (size_t i = 0; i < capacity_; ++i)
        {
            // stamp最低位为1代表当前槽位存在已构造的有效元素，需要手动析构
            // 因为 data 是原始内存 (placement new 构造), 编译器不会自动调用析构
            if (buffer_[i].stamp.load(std::memory_order_relaxed) & 1)
            {
                std::destroy_at(&buffer_[i].data);
            }
        }
        delete[] buffer_;
    }

    // 禁用拷贝构造，无锁队列无法安全拷贝多线程状态
    LockFreeMPMCQueue(const LockFreeMPMCQueue&) = delete;
    // 禁用拷贝赋值
    LockFreeMPMCQueue& operator=(const LockFreeMPMCQueue&) = delete;

    /// @brief 移动构造，转移队列内部资源所有权，源队列置空
    /// @param other 待移动的源无锁队列
    LockFreeMPMCQueue(LockFreeMPMCQueue&& other) noexcept
        : buffer_(std::exchange(other.buffer_, nullptr))
        , capacity_(std::exchange(other.capacity_, 0))
        , mask_(std::exchange(other.mask_, 0))
        , head_(other.head_.load(std::memory_order_relaxed))
        , tail_(other.tail_.load(std::memory_order_relaxed))
    {}

    /// @brief 移动语义入队，避免元素拷贝
    /// @param item 待入队的右值元素
    /// @return 入队成功返回true；队列满返回false
    bool enqueue(T&& item)
    {
        return do_enqueue(std::move(item));
    }

    /// @brief 拷贝语义入队，支持左值元素入队
    /// @param item 待入队的左值元素
    /// @return 入队成功返回true；队列满返回false
    bool enqueue(const T& item)
    {
        return do_enqueue(item);
    }

    /// @brief 出队操作，从队列头部取出一个有效元素
    ///
    /// 步骤分解：
    ///   1. 读取 tail 获取当前消费位置
    ///   2. 计算槽位索引 tail & mask_, 读取 stamp
    ///   3. 奇偶检测: (tail & 1) == (stamp & 1) 表示槽位"生产者持有"（空）
    ///      - 如果 head <= tail, 说明队列确实空了, 返回 false
    ///      - 否则有生产者在写入中, 自旋等待
    ///   4. CAS 抢占 tail: tail → tail + 1 (原子地占有当前槽位)
    ///   5. 检查 stamp != pos (pos = CAS 前的 tail):
    ///      - 如果成立: 数据已就绪, std::move 取出, destroy_at 析构,
    ///                 写入 stamp = pos + capacity_ (交还给生产者), 返回 true
    ///      - 否则: 被其他消费者抢先消费完, 重试
    ///
    /// @param out 出队元素的接收引用
    /// @return 出队成功返回true；队列为空返回false
    bool dequeue(T& out)
    {
        size_t pos;
        Slot* slot{};

        while (true)
        {
            // 加载消费位置 tail，宽松内存序，仅做局部读取
            pos = tail_.load(std::memory_order_relaxed);
            // 通过掩码快速计算当前 tail 对应的槽位下标
            slot = &buffer_[pos & mask_];
            // 获取槽位的版本标记，memory_order_acquire 保证看到前序写操作的可见性
            // 即生产者写入的数据对本线程可见
            size_t stamp = slot->stamp.load(std::memory_order_acquire);

            // 判断位置奇偶与槽位 stamp 奇偶是否一致：
            // 一致 → 槽位当前处于"生产者持有"状态（空或等待生产者写入）
            // 不一致 → 槽位处于"消费者持有"状态（有数据可取）
            if ((pos & 1) == (stamp & 1))
            {
                // 生产位置 head 不大于消费位置 pos，说明确实没有更多元素了
                if (head_.load(std::memory_order_acquire) <= pos)
                    return false;
                // 否则 head > pos：有生产者已经 claim 了某个位置但还没写完，
                // 我们自旋等待生产者写完（很快，因为生产者总在 claim 后立即写入）
                continue;
            }

            // CAS 尝试更新消费位置 tail，抢占当前槽位的消费权限
            // compare_exchange_weak 在 true 时: pos 不变, tail_ 更新为 pos+1
            //                   在 false 时: pos 更新为 tail_ 最新值，重新循环
            if (!tail_.compare_exchange_weak(pos, pos + 1,
                std::memory_order_relaxed, std::memory_order_relaxed))
                continue;

            // stamp != pos 说明数据已被生产者写入
            // (生产者写入后会设置 stamp = pos + 1, 所以 stamp 必然不等于 pos)
            if (stamp != pos)
            {
                // 移动取出槽内元素
                out = std::move(slot->data);
                // 手动销毁槽内元素（placement new 构造，需要手动析构）
                std::destroy_at(&slot->data);
                // 更新槽位版本戳为 pos + capacity_，标记槽位已消费，
                // 奇偶位回到偶数态，通知生产者可以重新使用此槽位。
                // 注意不是 pos+1 而是 pos+capacity_，这是因为下一轮 producer
                // 到达此槽位时的 position 是 pos + capacity_。
                // memory_order_release 保证前面 data 的读取在消费者看来先于 stamp 写入
                slot->stamp.store(pos + capacity_, std::memory_order_release);
                return true;
            }
            // stamp == pos: 只有一种情况 - 被另一个消费者抢先一步读走了数据
            // (该消费者写入了 stamp = pos + capacity_，但我们已经过了 parity check)
            // 实际上这种情况不会越过 parity check，这里 break 是防御性代码
            break;
        }
        return false;
    }

    /// @brief 获取队列当前近似元素个数
    /// @note 多线程并发场景下仅为瞬时近似值，不能作为精准判空/判满依据
    /// @return 当前队列近似元素数量
    size_t size_approx() const noexcept
    {
        auto h = head_.load(std::memory_order_relaxed);
        auto t = tail_.load(std::memory_order_relaxed);
        return t > h ? t - h : 0;
    }

    /// @brief 获取队列最大容量
    /// @return 初始化设置的队列容量（2的幂次）
    size_t capacity() const noexcept { return capacity_; }

private:
    /// @brief 环形队列单个存储槽位结构体
    /// @details 采用64字节对齐做伪共享(false sharing)隔离。
    ///          伪共享是指多个 CPU 核频繁修改同一缓存行上的不同变量，
    ///          导致缓存一致性协议反复使缓存行失效，性能急剧下降。
    ///          64字节对齐保证 Slot 完全占据一个独立的缓存行。
    struct Slot
    {
        /// @brief 版本标记 (atomic stamp)
        ///
        /// 核心作用：
        ///   1. 解决 ABA 问题：stamp 单调递增，即使同一物理槽位被反复使用，
        ///      也能通过 stamp 值而非仅奇偶位区分不同轮次
        ///   2. 标记槽位状态：
        ///      - 与 position 同奇偶 = 空/生产者持有
        ///      - 与 position 异奇偶 = 满/消费者持有
        ///
        /// 生命周期：
        ///   初始: stamp = i (偶数)         → 空
        ///   生产: stamp = pos+1 (奇数)     → 满 (pos 为生产时的前驱位置)
        ///   消费: stamp = pos+capacity (偶数) → 空 (下一轮)
        alignas(64) std::atomic<size_t> stamp{0};
        /// @brief 队列存储的元素，采用 placement new / destroy_at 管理生命周期
        alignas(64) T data;
    };

    /// @brief 通用入队模板函数，统一处理左值、右值转发
    ///
    /// 步骤分解：
    ///   1. 读取 head 获取当前生产位置
    ///   2. 计算槽位索引 head & mask_, 读取 stamp
    ///   3. 奇偶检测: (head & 1) != (stamp & 1) 表示槽位"消费者持有"（满）
    ///      → 队列满，自旋等待
    ///   4. CAS 抢占 head: head → head + 1 (原子地占有当前槽位)
    ///   5. 检查 stamp == pos (pos = CAS 前的 head):
    ///      - 如果成立: 槽位确实空闲，placement new 构造元素，
    ///                 写入 stamp = pos + 1 (奇偶翻转，通知消费者)，返回 true
    ///      - 否则: 队列真的满了，结束并返回 false
    ///
    /// @tparam U 元素值类型，支持左值/右值引用
    /// @param item 待入队元素
    /// @return 入队成功返回 true，队列满返回 false
    template<typename U>
    bool do_enqueue(U&& item)
    {
        size_t pos;
        Slot* slot{};

        while (true)
        {
            // 读取生产位置 head（宽松内存序，只做局部读取）
            pos = head_.load(std::memory_order_relaxed);
            slot = &buffer_[pos & mask_];
            // 获取槽位版本标记，acquire 保证看到消费者释放操作的可见性
            size_t stamp = slot->stamp.load(std::memory_order_acquire);

            // 奇偶位不一致 => stamp 是奇数（消费者持有），槽位有数据未被消费
            // 队列满，自旋等待消费者腾出槽位
            if ((pos & 1) != (stamp & 1))
                continue;

            // CAS 抢占生产位置 head
            // 成功：pos 不变，head 变为 pos+1，此线程获得 pos 槽位
            // 失败：pos 更新为 head 最新值，重新循环
            if (!head_.compare_exchange_weak(pos, pos + 1,
                std::memory_order_relaxed, std::memory_order_relaxed))
                continue;

            // stamp == pos 表示槽位确实空闲（没有其他线程同时修改过）
            if (stamp == pos)
            {
                // 在槽位原始内存上用 placement new 就地构造元素
                // std::forward 保持左值/右值引用语义
                new (&slot->data) T(std::forward<U>(item));
                // 更新 stamp = pos + 1，奇偶翻转，标记为"已满"，
                // memory_order_release 保证 data 写入在消费者 read 之前可见
                slot->stamp.store(pos + 1, std::memory_order_release);
                return true;
            }
            // 走到这里：stamp != pos 但奇偶位匹配
            // 说明另一个生产者在更早的轮次写入了此槽位（stamp 已被修改），
            // 而 CAS 成功后我们发现这实际上不是一个空闲槽位 → 队列真满
            break;
        }
        return false;
    }

    /// 环形队列槽位数组首地址
    Slot* buffer_{nullptr};
    /// 队列最大容量（必须为2的幂次）
    size_t capacity_{0};
    /// 索引掩码，用于快速取模：index = pos & mask_，等价于 pos % capacity_
    /// 例如 capacity=8, mask=7, pos=9 → 9 & 7 = 1
    size_t mask_{0};
    /// @brief 生产者位置计数器
    ///
    /// 单调递增，永不回绕（实际只受 64 位地址空间限制）。
    /// 每个生产者尝试 CAS head_ 来 claim 一个槽位。
    /// head_ & mask_ 得到当前槽位在环形数组中的物理下标。
    /// alignas(64) 将其隔离在独立缓存行，避免与 tail_ 伪共享。
    alignas(64) std::atomic<size_t> head_{0};
    /// @brief 消费者位置计数器
    ///
    /// 与 head_ 对称，每个消费者尝试 CAS tail_ 来 claim 一个槽位。
    /// 当 tail_ == head_ 且槽位为空时，队列为空。
    /// alignas(64) 将其隔离在独立缓存行，避免与 head_ 伪共享。
    alignas(64) std::atomic<size_t> tail_{0};
};
