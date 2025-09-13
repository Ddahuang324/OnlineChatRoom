#include "../../include/Sever/HTTPRequest.hpp"
#include "../../include/MiniEventLog.hpp"
#include "../../include/Platform.hpp"
#include "../../include/ConnectionManager.hpp"
#include <sstream>
#include <regex>
#include <chrono>
#include <memory>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

namespace {
    // 设置socket为非阻塞模式
    bool setNonBlocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0) return false;
        flags |= O_NONBLOCK;
        return fcntl(fd, F_SETFL, flags) >= 0;
    }

    // 解析URL，提取host、port和path
    bool parseURL(const std::string& url, std::string& host, int& port, std::string& path) {
        // 简单的URL解析：http://host:port/path 或 http://host/path
        std::regex url_regex(R"(^https?://([^:/]+)(?::(\d+))?(/.*)?$)");
        std::smatch matches;
        
        if (!std::regex_match(url, matches, url_regex)) {
            return false;
        }
        
        host = matches[1].str();
        port = matches[2].matched ? std::stoi(matches[2].str()) : 80;
        path = matches[3].matched ? matches[3].str() : "/";
        
        return true;
    }

    // 解析域名获取IP地址
    std::string getHostByName(const std::string& hostname) {
        struct hostent* host_entry = gethostbyname(hostname.c_str());
        if (host_entry == nullptr) {
            return "";
        }
        
        return std::string(inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0])));
    }

    // 构建HTTP POST请求报文
    std::string buildHttpPostRequest(const std::string& host, const std::string& path, 
                                   const std::string& post_data) {
        std::ostringstream request;
        request << "POST " << path << " HTTP/1.1\r\n";
        request << "Host: " << host << "\r\n";
        request << "Content-Type: application/x-www-form-urlencoded\r\n";
        request << "Content-Length: " << post_data.length() << "\r\n";
        request << "Connection: close\r\n";
        request << "\r\n";
        request << post_data;
        
        return request.str();
    }

    // 解析HTTP响应，检查状态码
    bool parseHttpResponse(const std::string& response, int& status_code) {
        std::regex status_regex(R"(^HTTP/\d\.\d\s+(\d+))");
        std::smatch matches;
        
        if (std::regex_search(response, matches, status_regex)) {
            status_code = std::stoi(matches[1].str());
            return true;
        }
        
        return false;
    }
}

HttpRequest::HttpRequest(EventBase* base)
    : base_(base), bev_(nullptr), timer_(base, -1), port_(80), connected_(false) {
    log_debug("HttpRequest created");
    
    // 增加连接计数
    ConnectionManager::getInstance().connectionIncrement();
}

HttpRequest::~HttpRequest() {
    log_debug("HttpRequest destroyed");
    
    // 减少连接计数
    ConnectionManager::getInstance().connectionDecrement();
}

void HttpRequest::postHttpRequest(const std::string& url, const std::string& post_data, user_callback cb) {
    // 增加请求计数
    ConnectionManager::getInstance().requestIncrement();
    
    // 保存用户回调和POST数据
    user_cb_ = std::move(cb);
    post_data_ = post_data;
    
    // 简单清理URL首尾空白
    auto trim = [](const std::string& s){
        size_t b = s.find_first_not_of(" \t\r\n");
        if(b==std::string::npos) return std::string();
        size_t e = s.find_last_not_of(" \t\r\n");
        return s.substr(b, e-b+1);
    };
    std::string cleanUrl = trim(url);
    // 解析URL
    if (!parseURL(cleanUrl, host_, port_, path_)) {
        log_error("Failed to parse URL: %s", url.c_str());
        if (user_cb_) {
            user_cb_(CONNECTION_EXCEPTION, nullptr);
        }
        return;
    }
    path_ = trim(path_);
    if(path_.empty()) path_ = "/";
    
    log_info("Parsed URL - Host: %s, Port: %d, Path: %s", host_.c_str(), port_, path_.c_str());
    
    // 解析域名获取IP地址
    std::string ip = getHostByName(host_);
    if (ip.empty()) {
        log_error("Failed to resolve hostname: %s", host_.c_str());
        if (user_cb_) {
            user_cb_(CONNECTION_EXCEPTION, nullptr);
        }
        return;
    }
    
    log_debug("Resolved %s to %s", host_.c_str(), ip.c_str());
    
    // 创建socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_error("Failed to create socket: %s", strerror(errno));
        if (user_cb_) {
            user_cb_(CONNECTION_EXCEPTION, nullptr);
        }
        return;
    }
    
    // 设置为非阻塞模式
    if (!setNonBlocking(sockfd)) {
        log_error("Failed to set socket non-blocking");
        close(sockfd);
        if (user_cb_) {
            user_cb_(CONNECTION_EXCEPTION, nullptr);
        }
        return;
    }
    
    // 创建BufferEvent
    bev_ = std::make_shared<BufferEvent>(base_, sockfd);
    
    // 设置回调函数
    bev_->setReadCallback([this](const BufferEvent::Ptr& bev, Buffer* input) {
        this->onRead(bev, input);
    });
    
    bev_->setWriteCallback([this](const BufferEvent::Ptr& bev) {
        log_debug("Write callback triggered");
    });
    
    bev_->setErrorCallback([this](const BufferEvent::Ptr& bev) {
        this->onError();
    });
    
    bev_->setCloseCallback([this](const BufferEvent::Ptr& bev) {
        log_debug("Connection closed");
    });
    
    // 暂时简化超时处理 - 在实际项目中可以使用EventBase的定时器功能
    // timer_.setTimeout(current_time + timeout_ms);
    // timer_.setTimerCallback([this]() {
    //     this->onTimeout();
    // });
    
    // 开始连接
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        log_error("Invalid IP address: %s", ip.c_str());
        if (user_cb_) {
            user_cb_(CONNECTION_EXCEPTION, nullptr);
        }
        return;
    }
    
    // 尝试连接
    int result = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (result < 0) {
        if (errno == EINPROGRESS) {
            // 非阻塞连接正在进行中
            log_debug("Connection in progress...");
            
            // 监听写事件来检测连接是否成功
            bev_->setWriteCallback([this](const BufferEvent::Ptr& bev) {
                // 检查连接是否成功
                int error = 0;
                socklen_t len = sizeof(error);
                if (getsockopt(bev->fd(), SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
                    log_error("Connection failed: %s", strerror(error));
                    this->onError();
                } else {
                    if(!connected_){
                        log_debug("Connection established successfully");
                        // 切换写回调为普通写完成通知，避免重复执行连接逻辑
                        this->onConnect(true);
                        bev->setWriteCallback([this](const BufferEvent::Ptr&){
                            log_debug("Write callback triggered");
                        });
                    }
                }
            });
            
            // 启用写事件监听，连接完成时会触发写事件
            bev_->enableWriting();
        } else {
            log_error("Connect failed immediately: %s", strerror(errno));
            if (user_cb_) {
                user_cb_(CONNECTION_EXCEPTION, nullptr);
            }
            return;
        }
    } else {
        // 连接立即成功（不太可能，但要处理这种情况）
        log_debug("Connection established immediately");
        bev_->connectEstablished();
        onConnect(true);
    }
}

void HttpRequest::onConnect(bool connected) {
    if (!connected) {
        log_error("Connection failed");
        if (user_cb_) {
            user_cb_(CONNECTION_EXCEPTION, nullptr);
        }
        return;
    }
    if(connected_) return; // 避免重复
    connected_ = true;
    
    log_debug("Connected successfully, sending HTTP request");
    
    // 构建HTTP POST请求
    std::string request = buildHttpPostRequest(host_, path_, post_data_);
    log_debug("HTTP Request:\n%s", request.c_str());
    
    // 确保已经监听读事件
    if(bev_) bev_->connectEstablished();
    // 发送请求
    bev_->write(request.data(), request.length());
}

void HttpRequest::onRead(BufferEvent::Ptr bev, Buffer* input) {
    log_debug("Received %zu bytes", input->readableBytes());
    
    // 读取所有可用数据
    std::string response = input->retrieveAllAsString();
    log_debug("HTTP Response:\n%s", response.c_str());
    
    // 解析HTTP响应状态码
    int status_code = 0;
    bool parse_success = parseHttpResponse(response, status_code);
    
    E_RESPONSE_TYPE response_type;
    if (!parse_success) {
        log_error("Failed to parse HTTP response");
        response_type = RESPONSE_ERROR;
    } else if (status_code >= 200 && status_code < 300) {
        log_info("HTTP request successful, status code: %d", status_code);
        response_type = RESPONSE_OK;
        // 增加响应计数
        ConnectionManager::getInstance().responseIncrement();
    } else {
        log_warn("HTTP request failed with status code: %d", status_code);
        response_type = RESPONSE_ERROR;
    }
    
    // 将响应数据写回Buffer供用户使用
    if (user_cb_) {
        Buffer* response_buffer = new Buffer();
        response_buffer->append(response.data(), response.size());
        user_cb_(response_type, response_buffer);
        delete response_buffer;
    }
    
    // 请求完成，关闭连接
    bev_->shutdown();
}

void HttpRequest::onError() {
    log_error("BufferEvent error occurred");
    
    if (user_cb_) {
        user_cb_(CONNECTION_EXCEPTION, nullptr);
    }
}

void HttpRequest::onTimeout() {
    log_warn("HTTP request timeout");
    
    // 增加超时响应计数
    ConnectionManager::getInstance().timeoutResponseIncrement();
    
    if (user_cb_) {
        user_cb_(REQUEST_TIMEOUT, nullptr);
    }
    
    // 超时后关闭连接
    if (bev_) {
        bev_->shutdown();
    }
}
