#pragma once

#include "../../include/EventBase.hpp"
#include "../../include/Buffer/BufferEvent.hpp"
#include <string>
#include <functional>

// 定义响应类型和用户回调函数指针
enum E_RESPONSE_TYPE { CONNECTION_EXCEPTION, REQUEST_TIMEOUT, RESPONSE_ERROR, RESPONSE_OK };
using user_callback = std::function<void(E_RESPONSE_TYPE, Buffer*)>;

class HttpRequest : public std::enable_shared_from_this<HttpRequest> {
public:
    HttpRequest(EventBase* base);
    ~HttpRequest();

    // 发起POST请求的核心方法
    void postHttpRequest(const std::string& url, const std::string& post_data, user_callback cb);

private:
    // BufferEvent 的一系列回调
    void onConnect(bool connected);
    void onRead(BufferEvent::Ptr bev, Buffer* input);
    void onError();

    // 超时处理
    void onTimeout();

    EventBase* base_;
    BufferEvent::Ptr bev_; // 使用智能指针管理 BufferEvent
    Channel timer_; // 用于处理超时

    std::string host_;
    int port_;
    std::string path_;
    std::string post_data_;

    user_callback user_cb_;
    bool connected_ = false; // 防止重复连接处理
};
