#pragma once

#include "../../include/AbstractSever.hpp"
#include "../../include/EventBase.hpp"
#include <memory>

class Channel;

class TimeoutServer : public AbstractServer {
public:
    TimeoutServer(EventBase* base);
    virtual ~TimeoutServer();

    // 实现基类方法
    // 端口参数在这里是无用的，但为了保持接口一致性，我们保留它
    virtual int listen(int dummy_port = 0) override;

    // TimeoutServer 不需要消息处理器，返回nullptr
    virtual MessageHandler* createMsgHandler() override;

private:
    // 定时器触发时调用的回调函数
    void onTimer();

    EventBase* base_;
    std::unique_ptr<Channel> timer_channel_;
    int counter_; // 用于在日志中计数，展示定时器在持续工作
};
