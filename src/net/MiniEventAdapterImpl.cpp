// Placeholder for the MiniEventAdapter implementation as per T023.
// Implementation will be provided by the user.
#include "net/MiniEventAdapterImpl.h"
#include "../../MiniEventWork/include/EventBase.hpp"
#include "../../MiniEventWork/include/Buffer/BufferEvent.hpp"
#include "../../MiniEventWork/include/EVConnConnector.hpp"
#include "common/Log.hpp"
#include <arpa/inet.h>

MiniEventAdapterImpl::MiniEventAdapterImpl() = default;

MiniEventAdapterImpl::~MiniEventAdapterImpl(){
    reconnecting_ = false; // 停止重连
    if (isInternalEventBase_ .load()){
        // 请求事件循环退出
        if(eventBase_){
            eventBase_->quit();
        }
        if(eventThread_ && eventThread_->joinable()){
            eventThread_->join();
        }
    }
}


void MiniEventAdapterImpl::init(MiniEventWork::EventBase* eventBase,ConnectionCallback connCb,MessageCallback msgCb,FileChunkCallback fileCb){
   
    connectionCallback_ = std::move(connCb);
    messageCallback_ = std::move(msgCb);
    fileCallback_ = std::move(fileCb);


    if (eventBase){
        eventBase_ = eventBase;
        isInternalEventBase_ = false;
        return;
    }

    // 内部创建 EventBase 与线程
    eventBase_ = new MiniEventWork::EventBase();
    isInternalEventBase_ = true;
    eventThread_ = std::make_unique<std::thread>([this]{
        if (eventBase_){
            log_debug("EventBase loop start.");
            eventBase_->loop();
            log_debug("EventBase loop exited.");
        }
    });
}


void MiniEventAdapterImpl::connect(const ConnectionParams& params){
    if(!eventBase_){
        log_error("EventBase not initialized in MiniEventAdapterImpl::connect");
        return;
    }
    // 保存连接参数用于重连
    lastParams_ = params;
    // 如果已有连接，先释放
    if (bev_) {
        bev_->shutdown();
        bev_.reset();
    }

    // 构建 lambda，捕获 this 和 params
    auto connectLambda = [this, params]() {
        // 创建连接器并发起连接（现在在 I/O 线程）
        connector_ = std::make_unique<MiniEventWork::EVConnConnector>(eventBase_, params, [this](bool success, int sockfd) {
            if (success) {
                // 连接成功，创建 BufferEvent
                bev_ = std::make_shared<MiniEventWork::BufferEvent>(eventBase_, sockfd);
                MiniEventWork::BufferEvent::setCallBacks(bev_,
                    [this] { this->onRead(); },
                    []() {},
                    [this](short events) { this->onEvent(events); }
                );
                MiniEventWork::BufferEvent::enable(bev_, MiniEventWork::BufferEvent::kReadEvent | MiniEventWork::BufferEvent::kWriteEvent);
                // 重连成功，重置状态
                reconnecting_ = false;
                retryCount_ = 0;
                backoffTime_ = std::chrono::milliseconds(1000);
                if (connectionCallback_) {
                    connectionCallback_(connectionEvents::Connected);
                }
            } else {
                // 连接失败
                if (connectionCallback_) {
                    connectionCallback_(connectionEvents::ConnectFailed);
                }
            }
        });
    };

    // 将 lambda 推送到 I/O 线程执行
    eventBase_->queueInLoop(connectLambda);
}

void MiniEventAdapterImpl::disconnect(){
    if( !eventBase_){
        log_error("EventBase not initialized in MiniEventAdapterImpl::disconnect");
        return;
    }
    // 停止重连
    reconnecting_ = false;
    retryCount_ = 0;
    backoffTime_ = std::chrono::milliseconds(1000);
    eventBase_->queueInLoop([this]{
        if(bev_){
            log_debug("disconnecting");
            bev_->shutdown();
            bev_.reset();
            log_debug("disconnected");
        }
    });
}

void MiniEventAdapterImpl::onRead() {
    if (!bev_ || !messageCallback_) return;

    auto& inputBuffer = bev_->getInputBuffer();
    while (true) {
        if (inputBuffer.readableBytes() < sizeof(uint32_t)) {
            // 不足以读取长度前缀
            break;
        }
        uint32_t bodylen;

        memcpy(&bodylen, inputBuffer.peek(), sizeof(bodylen));
        bodylen = ntohl(bodylen);

        if (inputBuffer.readableBytes() < sizeof(uint32_t) + bodylen) {
            // 不足以读取完整消息
            break;
        }

        inputBuffer.retrieve(sizeof(uint32_t)); // 移动读指针，跳过长度前缀

        SerializedMessage serialized(bodylen);
        memcpy(serialized.data(), inputBuffer.peek(), bodylen);
        inputBuffer.retrieve(bodylen);

        MessageRecord msg;
        if (deserializeMessage(serialized, msg)) {
            messageCallback_(msg);
        } else {
            log_error("Failed to deserialize message");
            disconnect();
            break;
        }
    }


  
}

void MiniEventAdapterImpl::onEvent(short events) {
    if (!connectionCallback_) return;

    if (events & MiniEventWork::BufferEvent::kEOFEvent) {
        // Connection closed
        connectionCallback_(connectionEvents::Disconnected);
        bev_.reset();
        // 触发重连，如果未在重连中
        if (!reconnecting_.load()) {
            reconnecting_ = true;
            eventBase_->runAfter(backoffTime_, [this]() { attemptReconnect(); });
        }
    } else if (events & MiniEventWork::BufferEvent::kErrorEvent) {
        // Connection error
        connectionCallback_(connectionEvents::Disconnected);
        bev_.reset();
        // 触发重连，如果未在重连中
        if (!reconnecting_.load()) {
            reconnecting_ = true;
            eventBase_->runAfter(backoffTime_, [this]() { attemptReconnect(); });
        }
    }
}

void MiniEventAdapterImpl::sendMessage(const MessageRecord& msg) {
    if(!eventBase_){
        log_error("EventBase not initialized in MiniEventAdapterImpl::sendMessage");
        return;
    }

   SerializedMessage serialized = serializedMessage(msg);
   {
    std::lock_guard<std::mutex> lock(sendQueueMutex_);
    sendQueue_.push_back(serialized);
   }
    eventBase_->queueInLoop([this] { processSendQueue(); });
}

void MiniEventAdapterImpl::processSendQueue() {
    std::deque<SerializedMessage> toSend;
    {
        std::lock_guard<std::mutex> lock(sendQueueMutex_);
        toSend.swap(sendQueue_);
    }

    if(bev_){
        for (const auto& serialized : toSend) {
            uint32_t len = htonl(static_cast<uint32_t>(serialized.size()));
            bev_->write(&len, sizeof(len));
            bev_->write(serialized.data(), serialized.size());
        }
    }else {
        log_warn("BufferEvent not connected, cannot send messages");
    }
 }

void MiniEventAdapterImpl::sendFileChunk(const FileMeta& chunk) {
    // Placeholder: not implemented yet
    // For now, do nothing
}

