#include "../../include/Sever/TimeOutServer.hpp"  // 修正文件名大小写与拼写
#include "../../include/Channel.hpp"
#include "../../include/MiniEventLog.hpp"
#include <chrono>

TimeoutServer::TimeoutServer(EventBase* base)
    : base_(base), timer_channel_(nullptr), counter_(0) {
    log_info("TimeoutServer created.");
}

TimeoutServer::~TimeoutServer() {
    log_info("TimeoutServer destroyed.");
    // unique_ptr 会自动释放 Channel 资源
}

// listen 方法的职责在这里从“监听网络”变为了“启动定时器”
int TimeoutServer::listen(int dummy_port) {
    // 1. 创建一个不关联任何文件描述符的 Channel，专门用于定时
    timer_channel_ = std::make_unique<Channel>(base_, -1);

    // 2. 设置定时器回调
    timer_channel_->setTimerCallback([this]() {
        this->onTimer();
    });

    log_info("TimeoutServer 'listens' by starting its timer.");

    // 3. 立即触发第一次回调，启动定时循环
    onTimer();

    return 0;
}

// 这个服务不处理任何外部消息，所以它不需要MessageHandler
MessageHandler* TimeoutServer::createMsgHandler() {
    return nullptr;
}

void TimeoutServer::onTimer() {
    // 4. 打印日志，证明定时器被触发
    log_info("--- TimeoutServer Tick! Count: %d ---", ++counter_);

    // 5. 计算下一次的超时时间（从当前时间开始的3秒后）
    //    并重新将 Channel 注册（或更新）到 EventBase 的最小堆中
    // 不能直接访问 EventBase 的私有时间函数，这里自行获取当前时间
    auto now = std::chrono::steady_clock::now();
    uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    uint64_t next_timeout_ms = now_ms + 3000;
    timer_channel_->setTimeout(next_timeout_ms);
    base_->updateChannel(timer_channel_.get());
}
