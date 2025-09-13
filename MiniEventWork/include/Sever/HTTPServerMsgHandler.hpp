#pragma once
#include "../../include/MessageHandler.hpp"
#include <memory> // for std::shared_ptr

class BufferEvent;

class HttpServerMsgHandler : public MessageHandler {
public:
    HttpServerMsgHandler() = default;
    virtual ~HttpServerMsgHandler() = default;
    // 实现基类的纯虚函数，用于处理HTTP请求
    virtual void handleMessage(void* arg) override;
};
