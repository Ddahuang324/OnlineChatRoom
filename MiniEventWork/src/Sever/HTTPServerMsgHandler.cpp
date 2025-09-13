#include "../../include/Sever/HTTPServerMsgHandler.hpp"
#include "../../include/Sever/HTTPServer.hpp" // For HttpRequestContext
#include "../../include/Buffer/BufferEvent.hpp"
#include "../../include/ConnectionManager.hpp"
#include <sstream>

void HttpServerMsgHandler::handleMessage(void* arg) {
    // 将void*参数安全地转回我们定义的类型
    auto* request_pair = static_cast<std::pair<HttpRequestContext, std::shared_ptr<BufferEvent>>*>(arg);
    const auto& req_ctx = request_pair->first;
    const auto& bev = request_pair->second;

    // --- 业务逻辑：根据请求路径(path)选择不同的操作 ---
    std::string response_body;
    if (req_ctx.path == "/statistic") {
        // 获取统计数据
    ConnectionManager& cm = ConnectionManager::getInstance();
        std::stringstream body_ss;
        body_ss << "<html><head><title>Server Statistics</title></head><body>"
                << "<h1>Server Statistics</h1>"
        << "<p>Total Requests: " << cm.getTotalRequest() << "</p>"
        << "<p>Total Responses: " << cm.getTotalResponse() << "</p>"
        << "<p>Active Connections: " << cm.getConnections() << "</p>"
        << "<p>Timeout Responses: " << cm.getTimeoutResponse() << "</p>"
                << "</body></html>";
        response_body = body_ss.str();

    } else {
        // 404 Not Found
        response_body = "<html><body><h1>404 Not Found</h1></body></html>";
    }

    // --- 构建HTTP响应报文 ---
    std::stringstream response_ss;
    response_ss << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: text/html; charset=utf-8\r\n"
                << "Content-Length: " << response_body.length() << "\r\n"
                << "Connection: close\r\n"
                << "\r\n" // 关键的空行
                << response_body;

    std::string http_response = response_ss.str();

    // --- 通过BufferEvent发送响应 ---
    bev->write(http_response.data(), http_response.length());

}
