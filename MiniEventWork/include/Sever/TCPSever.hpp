#pragma once

#include <memory>
#include "../../include/Buffer/BufferEvent.hpp"
#include "../../include/EVConnListener.hpp"
#include "../../include/EventBase.hpp"  
#include "../../include/AbstractSever.hpp"

class EVConnListener;

class TCPSever :public AbstractServer{
public:
    TCPSever(EventBase* base);
    virtual ~TCPSever();

    
    virtual int listen(int port) override;

    virtual MessageHandler* createMsgHandler() override;

private:
    void onNewConnection(int sockfd, const struct sockaddr* addr, socklen_t len);
    EventBase* loop_;
    std::unique_ptr<EVConnListener> listener_;
    // 保存所有连接，防止BufferEvent提前析构
    std::vector<std::shared_ptr<BufferEvent>> connections_;
};