#include "../../include/Sever/HTTPServer.hpp"
#include "../../include/Sever/HTTPServerMsgHandler.hpp"
#include "../../include/EVConnListener.hpp"
#include "../../include/Buffer/BufferEvent.hpp"
#include "../../include/MiniEventLog.hpp"
#include <sstream>

HttpServer::HttpServer(EventBase* base)
    : base_(base), listener_(nullptr) {
}

HttpServer::~HttpServer() {
}

int HttpServer::listen(int port) {
    // 创建监听器，回调绑定到 onNewConnection 成员函数
    listener_ = std::make_unique<EVConnListener>(base_,
        [this](int fd, const sockaddr*, socklen_t){ this->onNewConnection(fd); });

    log_info("HTTP Server starts listening on port %d", port);
    return listener_->listen(port);
}

// 工厂方法，创建对应的HttpServerMsgHandler
MessageHandler* HttpServer::createMsgHandler() {
    return new HttpServerMsgHandler();
}

void HttpServer::onNewConnection(int sockfd) {
    log_info("Accepted new HTTP connection with fd = %d", sockfd);

    // 1. 为新连接创建一个BufferEvent
    auto bev = std::make_shared<BufferEvent>(base_, sockfd);

    // 2. 设置读回调，这是HTTP服务器的核心
    //    当数据到达时，onMessage 方法将被调用
    bev->setReadCallback(
        [this, bev](const BufferEvent::Ptr&, Buffer* buf) {
            this->onMessage(bev, buf);
        }
    );

    // 3. 启用事件监听
    bev->connectEstablished();
}

void HttpServer::onMessage(const std::shared_ptr<BufferEvent>& bev, Buffer* buffer) {
    // --- 极简HTTP请求解析器 ---
    // 查找请求头结束的标志："\r\n\r\n"
    const char* crlf_crlf = buffer->find_str("\r\n\r\n");

    if (crlf_crlf == nullptr) {
        // 数据不完整，等待更多数据
        log_debug("HTTP request headers not complete yet.");
        return;
    }

    // 找到了请求头末尾，开始解析
    // 1. 解析请求行 (e.g., "GET /statistic HTTP/1.1")
    const char* start = buffer->peek();
    const char* crlf = buffer->find_str("\r\n");
    if (crlf == nullptr) return; // 格式错误

    std::string request_line(start, crlf - start);

    // 使用 stringstream 简化解析
    std::stringstream ss(request_line);
    HttpRequestContext req_ctx;
    ss >> req_ctx.method >> req_ctx.path;

    // 检查解析是否成功
    if (req_ctx.method.empty() || req_ctx.path.empty()) {
        log_error("Failed to parse HTTP request line: %s", request_line.c_str());
        bev->shutdown(); // 关闭格式错误的连接
        return;
    }

    // T026: 检查方法是否支持
    if (req_ctx.method != "GET") {
        log_warn("Unsupported HTTP method: %s", req_ctx.method.c_str());
        std::string response_body = "<html><body><h1>501 Not Implemented</h1></body></html>";
        std::stringstream response_ss;
        response_ss << "HTTP/1.1 501 Not Implemented\r\n"
                    << "Content-Type: text/html; charset=utf-8\r\n"
                    << "Content-Length: " << response_body.length() << "\r\n"
                    << "Connection: close\r\n"
                    << "\r\n"
                    << response_body;
        std::string http_response = response_ss.str();
        bev->write(http_response.data(), http_response.length());
        bev->shutdown(); // 发送后关闭
        return;
    }

    log_info("Received HTTP Request: Method=%s, Path=%s", req_ctx.method.c_str(), req_ctx.path.c_str());

    // 2. 将解析后的请求上下文和用于响应的BufferEvent指针，一起交给MessageHandler处理
    MessageHandler* handler = getMsgHandler();
    if (handler) {
        // 创建一个在栈上持久存在于本函数调用期间的 pair，并传递其地址
        auto ctx_pair = std::make_pair(req_ctx, bev);
        handler->handleMessage(&ctx_pair);
    }

    // 我们的极简服务器是短连接，处理完一个请求就关闭
    // 在一个完整的实现中，会根据HTTP头的 "Connection" 字段来决定是否保持连接
    bev->shutdown();
}
