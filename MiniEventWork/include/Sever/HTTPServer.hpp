#pragma once

#include "../../include/AbstractSever.hpp"
#include "../../include/EventBase.hpp"
#include <memory>
#include <string>

// 前向声明，减少头文件依赖
class EVConnListener;
class BufferEvent;
class Buffer;

// 这是一个简化的HTTP请求上下文结构体
// 用于在解析后，将请求信息传递给MessageHandler
struct HttpRequestContext {
    std::string method;
    std::string path;
    // 在更复杂的实现中，这里还会有头部(headers)和正文(body)
};

class HttpServer : public AbstractServer {
public:
    HttpServer(EventBase* base);
    virtual ~HttpServer();

    // 实现基类的纯虚函数
    virtual int listen(int port) override;
    virtual MessageHandler* createMsgHandler() override;

private:
    // 处理新连接的回调
    void onNewConnection(int sockfd);

    // 关键：处理并解析来自客户端的数据
    void onMessage(const std::shared_ptr<BufferEvent>& bev, Buffer* buffer);

    EventBase* base_;
    std::unique_ptr<EVConnListener> listener_;
};
