#pragma once

#include <functional>
#include <memory>
#include <string>
#include "../include/Channel.hpp"
#include "../include/EventBase.hpp"
#include "../../include/net/MiniEventAdapter.h"  // 为了使用 ConnectionParams

class EVConnConnector
{
public:
    using ConnectionCallback = std::function<void(bool success, int sockfd)>;
    EVConnConnector(MiniEventWork::EventBase* loop, ConnectionCallback&& connection_callback);
    // 新增重载构造函数，支持传入 ConnectionParams
    EVConnConnector(MiniEventWork::EventBase* loop, const ConnectionParams& params, ConnectionCallback&& connection_callback);
    ~EVConnConnector();

    EVConnConnector(const EVConnConnector&) = delete;
    EVConnConnector& operator=(const EVConnConnector&) = delete;

    int connect(const std::string& host, int port);

private:
    void handleConnect();
    void retry(int sockfd);

    std::unique_ptr<MiniEventWork::Channel> channel_;
    MiniEventWork::EventBase* loop_;
    ConnectionCallback connection_callback_;
    bool connecting_;
    std::string host_;
    int port_;
    int retry_delay_ms_;
};

