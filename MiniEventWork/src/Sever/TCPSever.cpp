#include "../../include/Sever/TCPSever.hpp"

#include "../../include/MiniEventLog.hpp"
#include "../../include/Buffer/BufferEvent.hpp"


TCPSever::TCPSever(EventBase* base)
    : loop_(base), listener_(nullptr) {
    listener_ = std::make_unique<EVConnListener>(loop_, 
        std::bind(&TCPSever::onNewConnection, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

TCPSever::~TCPSever() {
    listener_.reset();
}

int TCPSever::listen(int port) {
    return listener_->listen(port);
}

MessageHandler* TCPSever::createMsgHandler() {
    // 这里可以返回一个具体的 MessageHandler 实例
    // MessageHandler 是抽象类，需要返回具体的子类实例或 nullptr
    return nullptr;
}

void TCPSever::onNewConnection(int sockfd, const struct sockaddr* addr, socklen_t len) {
    // 处理新连接，创建BufferEvent并设置echo回调
    log_info("New connection accepted, sockfd: %d", sockfd);
    auto bev = std::make_shared<BufferEvent>(loop_, sockfd);
    bev->setReadCallback([this](const BufferEvent::Ptr& bevPtr, Buffer* buf){
        std::string data(buf->peek(), buf->readableBytes());
        buf->retrieve(buf->readableBytes());
        bevPtr->write(data.data(), data.size());
    });
    bev->setCloseCallback([this](const BufferEvent::Ptr& bevPtr){
        log_info("Connection closed: %d", bevPtr->fd());
        // 移除已关闭连接
        auto it = std::remove_if(connections_.begin(), connections_.end(), [&](const std::shared_ptr<BufferEvent>& ptr){
            return ptr->fd() == bevPtr->fd();
        });
        connections_.erase(it, connections_.end());
    });
    bev->connectEstablished();
    connections_.push_back(bev);
}

