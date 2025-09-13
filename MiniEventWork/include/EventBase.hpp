#pragma once

#include "IO_Multiplexer.hpp"
#include "Channel.hpp"
#include "DataStructure/min_heap.hpp"
#include "Platform.hpp"

#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <cstdint>
#include "MultiThread/ThreadPool_副本.hpp"
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>



// [新增] 为最小堆定义比较器
// 比较两个 Channel 的绝对超时时间
struct ChannelTimeoutComparator
{
    bool operator()(const std::shared_ptr<Channel> &a, const std::shared_ptr<Channel> &b) const
    {
        return a->getTimeout() > b->getTimeout();
    }
};

class EventBase
{
public:
    EventBase(size_t thread_num = 0);
    ~EventBase();

    // 启动事件循环
    void loop();
    // 退出事件循环
    void quit();

    // 更新/添加 Channel
    void updateChannel(Channel *channel);
    // 移除 Channel
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // Test hook: observer invoked with fd when a connection times out.
    void setTimeoutObserver(std::function<void(int)> observer);

    // 线程相关：在事件循环线程执行函数；若在其它线程调用则排队并唤醒
    void runInLoop(const std::function<void()>& cb);
    void queueInLoop(const std::function<void()>& cb); // 总是排队
    bool isInLoopThread() const { return std::this_thread::get_id() == loop_thread_id_; }
    // 提交到工作线程池（仅业务计算），完成后应再回到 loop 线程做 I/O
    void executeInWorker(const std::function<void()>& task);

private:
    // [新增] 定时器相关私有方法
    void addTimer(std::shared_ptr<Channel> channel);
    void removeTimer(std::shared_ptr<Channel> channel);
    void handleExpiredTimers();
    uint64_t getNextTimeout() const;
    uint64_t getCurrentTimeMs() const;

    bool quit_;
    std::unique_ptr<IOMultiplexer> io_multiplexer_;
    std::map<int, std::shared_ptr<Channel>> channels_;
    std::vector<Channel*> active_channels_;

    // [新增] 定时器管理数据结构
    min_heap_t timer_heap_;
    std::vector<std::shared_ptr<Channel>> expired_timer_channels_;
    std::function<void(int)> timeout_observer_;

    // [新增] 线程池指针（可选）
    std::unique_ptr<ThreadPool> thread_pool_;
    size_t thread_num_;

    // =========== 新增：跨线程任务队列与唤醒机制 ===========
    std::thread::id loop_thread_id_;
    std::vector<std::function<void()>> pending_tasks_;
    std::mutex pending_mutex_;
    bool calling_pending_ = false;
    int wakeup_fds_[2] = {-1,-1}; // pipe[0]=read, pipe[1]=write
    std::unique_ptr<Channel> wakeup_channel_;
    void wakeup();
    void handleWakeup();
    void doPendingTasks();
};
