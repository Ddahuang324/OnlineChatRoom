// Placeholder for the MiniEventAdapter implementation as per T023.
// Implementation will be provided by the user.
#include "net/MiniEventAdapterImpl.h"
#include "../../MiniEventWork/include/EventBase.hpp"
#include "../../MiniEventWork/include/Buffer/BufferEvent.hpp"
#include "../../MiniEventWork/include/EVConnConnector.hpp"
#include "common/Log.hpp"
#include <nlohmann/json.hpp>
#include <arpa/inet.h>
#include <cstring>

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
    // connector_ 使用 unique_ptr 管理，直接 reset 即可；其自定义删除器会尝试在 eventBase_ 线程上删除对象
    if (connector_) {
        connector_.reset();
    }
}

void MiniEventAdapterImpl::init(MiniEventWork::EventBase* eventBase,ConnectionCallback connCb,MessageCallback msgCb,FileChunkCallback fileCb,LoginCallback loginCb){
   
    // 注意：MiniEventAdapterImpl 的对象线程亲和力在主线程，但其内部 I/O 事件在独立 eventBase_ 线程触发。
    // 直接在 I/O 线程里 emit 信号会导致：表面上是 DirectConnection（因为对象 threadAffinity = 主线程），
    // 但实际 slot 在错误线程执行，可能引发未定义行为（UI 无法刷新 / 随机崩溃）。
    // 这里统一通过 QMetaObject::invokeMethod(..., Qt::QueuedConnection) 把信号投递回对象所属线程。
    connectionCallback_ = [this, connCb](connectionEvents event) {
        if (connCb) connCb(event);
        QMetaObject::invokeMethod(this, [this, event]() { emit connectionEvent(event); }, Qt::QueuedConnection);
    };
    messageCallback_ = [this, msgCb](const MessageRecord& msg) {
        if (msgCb) msgCb(msg);
        QMetaObject::invokeMethod(this, [this, msg]() { emit messageReceived(msg); }, Qt::QueuedConnection);
    };
    fileCallback_ = std::move(fileCb);
    loginCallback_ = [this, loginCb](const LoginResponse& resp) {
        if (loginCb) loginCb(resp);
        QMetaObject::invokeMethod(this, [this, resp]() { emit loginResponse(resp); }, Qt::QueuedConnection);
    };


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
        // 使用 unique_ptr 并传入自定义 deleter，确保删除在 eventBase_ 线程上执行
        auto deleter = [this](MiniEventWork::EVConnConnector* p){
            if (!p) return;
            if (eventBase_ && eventBase_->isInLoopThread()){
                delete p;
            } else if (eventBase_) {
                // 在 event loop 线程删除
                eventBase_->runInLoop([p]{ delete p; });
            } else {
                // 退化为直接删除
                delete p;
            }
        };
    connector_ = std::unique_ptr<MiniEventWork::EVConnConnector, std::function<void(MiniEventWork::EVConnConnector*)>>(new MiniEventWork::EVConnConnector(eventBase_, params, [this](bool success, int sockfd) {
            if (success) {
                // 连接成功，此时 sockfd 已经由 EVConnConnector 管理，
                // 我们需要接管它，并从 EVConnConnector 中移除
                auto channel = connector_->releaseChannel();
                if (!channel || channel->fd() != sockfd) {
                    // 如果 channel 获取失败或 fd 不匹配，则关闭 sockfd
                    ::close(sockfd);
                    if (connectionCallback_) {
                        connectionCallback_(connectionEvents::ConnectFailed);
                    }
                    return;
                }

                // 释放 unique_ptr 的所有权，交由自定义删除器在合适的线程执行删除
                // 这里先把 connector_ 置空，触发 deleter（会在 eventBase_ 线程上删除）
                connector_.reset();

                // 然后创建 BufferEvent（从 connector 接管 Channel 而不是重新创建）
                bev_ = std::make_shared<MiniEventWork::BufferEvent>(eventBase_, std::move(channel));
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
    }), deleter);
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
    if (!bev_ ) return;
    auto& inputBuffer = bev_->getInputBuffer();
    while (true) {
        const size_t headersize = 1 + sizeof(uint32_t); // 1 byte for type, 4 bytes for length
        if (inputBuffer.readableBytes() < headersize) {
            // 不足以读取长度前缀
            break;
        }
        uint8_t type;
        memcpy(&type, inputBuffer.peek(), 1);

        uint32_t bodylen;
        memcpy(&bodylen, inputBuffer.peek() + 1, sizeof(uint32_t));
        bodylen = ntohl(bodylen);

        size_t totalSize = headersize + bodylen;
        if (inputBuffer.readableBytes() < totalSize) {
            // 不足以读取完整消息
            break;
        }

        SerializedMessage serialized(totalSize);
        memcpy(serialized.data(), inputBuffer.peek(), totalSize);
        inputBuffer.retrieve(totalSize);

         // 5. [设计模式] 根据类型分发到不同的处理器
        switch (type) {
            case 0x01: { // Message
                if (messageCallback_) {
                    MessageRecord msg;
                    if (deserializeMessage(serialized, msg)) {
                        messageCallback_(msg);
                    } else {
                        log_error("Failed to deserialize Message. Disconnecting.");
                        disconnect();
                    }
                }
                break;
            }
            case 0x02: { // FileChunk
                if (fileCallback_) {
                    FileMeta chunk;
                    if (deserializeFileChunk(serialized, chunk)) {
                        fileCallback_(chunk);
                    } else {
                        log_error("Failed to deserialize FileChunk. Disconnecting.");
                        disconnect();
                    }
                }
                break;
            }
            case 0x03: { // LoginResponse
                if (loginCallback_) {
                    LoginResponse resp;
                    if (deserializeLoginResponse(serialized, resp)) {
                        loginCallback_(resp);
                    } else {
                        log_error("Failed to deserialize LoginResponse. Disconnecting.");
                        disconnect();
                    }
                }
                break;
            }
            default:
                log_error("Unknown message type received: %d. Disconnecting.", type);
                disconnect();
                break; // 必须跳出循环
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

   SerializedMessage frame = serializedMessage(msg);
   {
    std::lock_guard<std::mutex> lock(sendQueueMutex_);
    sendQueue_.push_back(frame);
   }
    eventBase_->queueInLoop([this] { processSendQueue(); });
}

void MiniEventAdapterImpl::processSendQueue() {
    std::deque<SerializedMessage> toSend;
    {
        std::lock_guard<std::mutex> lock(sendQueueMutex_);
        toSend.swap(sendQueue_);
    }

    log_debug("processSendQueue called with %zu messages", toSend.size());
    if(bev_){
        for (const auto& serialized : toSend) {
            log_debug("Writing %zu bytes to BufferEvent", serialized.size());
            bev_->write(serialized.data(), serialized.size());
            log_debug("Write completed");
        }
    }else {
        log_warn("BufferEvent not connected, dropping %zu messages", toSend.size());
    }
 }

void MiniEventAdapterImpl::sendFileChunk(const FileMeta& chunk) {
    if(!eventBase_){
        log_error("EventBase not initialized in MiniEventAdapterImpl::sendFileChunk");
        return;
    }
    SerializedMessage frame = serializedFileChunk(chunk);

    {
        std::lock_guard<std::mutex> lock(sendQueueMutex_);
        sendQueue_.push_back(frame);
    }
    eventBase_->queueInLoop([this] { processSendQueue(); });
}

void MiniEventAdapterImpl::sendLoginRequest(const LoginRequest& req) {
    log_debug("MiniEventAdapterImpl::sendLoginRequest called with username: %s", req.username.c_str());
    if(!eventBase_){
        log_error("EventBase not initialized in MiniEventAdapterImpl::sendLoginRequest");
        return;
    }
    SerializedMessage frame = serializedLoginRequest(req);
    log_debug("Serialized login request, size: %zu", frame.size());

    {
        std::lock_guard<std::mutex> lock(sendQueueMutex_);
        sendQueue_.push_back(frame);
        log_debug("Added to send queue, queue size: %zu", sendQueue_.size());
    }
    eventBase_->queueInLoop([this] { processSendQueue(); });
}

void MiniEventAdapterImpl::attemptReconnect() {
    if (!reconnecting_.load()) return;

    log_debug("Attempting to reconnect, retry count: %d", retryCount_);

    // 尝试连接
    connect(lastParams_);

    // 注意：connect 是异步的，成功会在 connect 的回调中处理
    // 这里不立即增加计数，失败时在下次 onEvent 中处理

    // 增加退避时间
    backoffTime_ = std::min(backoffTime_ * 2, std::chrono::milliseconds(60000)); // 最大60秒
    retryCount_++;

    // 如果超过最大重试次数，停止重连
    if (retryCount_ >= 5) {
        log_error("Max reconnection attempts reached, stopping reconnection");
        reconnecting_ = false;
        retryCount_ = 0;
        backoffTime_ = std::chrono::milliseconds(1000);
    }
}

SerializedMessage MiniEventAdapterImpl::serializedMessage(const MessageRecord msg){
    nlohmann::json j;
    j["id"] = msg.id;
    j["roomId"] = msg.roomId;
    j["senderId"] = msg.senderId;
    j["localTimestamp"] = msg.localTimestamp;
    j["serverTimestamp"] = msg.serverTimestamp;
    j["type"] = static_cast<int>(msg.type);  // 使用int表示enum
    j["content"] = msg.content;
    j["state"] = static_cast<int>(msg.state);
    if (msg.fileMeta) {
        nlohmann::json fileJ;
        fileJ["fileId"] = msg.fileMeta->fileId;
        fileJ["fileName"] = msg.fileMeta->fileName;
        fileJ["mimeType"] = msg.fileMeta->mimeType;
        fileJ["localPath"] = msg.fileMeta->localPath;
        fileJ["fileSize"] = msg.fileMeta->fileSize;
        fileJ["chunkIndex"] = msg.fileMeta->chunkIndex;
        fileJ["totalChunks"] = msg.fileMeta->totalChunks;
        j["fileMeta"] = fileJ;
    } else {
        j["fileMeta"] = nullptr;
    }

    std::string jsonStr = j.dump();
    std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());
    uint32_t length = htonl(static_cast<uint32_t>(payload.size()));
    SerializedMessage result;
    result.push_back(0x01);  // 类型字节 0x01 for Message
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&length), reinterpret_cast<uint8_t*>(&length) + sizeof(length));
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

bool MiniEventAdapterImpl::deserializeMessage(const SerializedMessage& serialized, MessageRecord& msg){
    if (serialized.size() < 1 + sizeof(uint32_t)) {
        return false;  // 不足以读取类型和长度
    }
    if (serialized[0] != 0x01) {
        return false;  // 类型不匹配
    }
    uint32_t length;
    memcpy(&length, &serialized[1], sizeof(length));
    length = ntohl(length);
    if (serialized.size() < 1 + sizeof(uint32_t) + length) {
        return false;  // 数据不足
    }
    std::string jsonStr(reinterpret_cast<const char*>(&serialized[1 + sizeof(uint32_t)]), length);
    try {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        msg.id = j.at("id").get<std::string>();
        msg.roomId = j.at("roomId").get<std::string>();
        msg.senderId = j.at("senderId").get<std::string>();
        msg.localTimestamp = j.at("localTimestamp").get<int64_t>();
        msg.serverTimestamp = j.at("serverTimestamp").get<int64_t>();
        msg.type = static_cast<MessageType>(j.at("type").get<int>());
        msg.content = j.at("content").get<std::string>();
        msg.state = static_cast<MessageState>(j.at("state").get<int>());
        if (!j.at("fileMeta").is_null()) {
            auto fileJ = j.at("fileMeta");
            msg.fileMeta = std::make_shared<FileMeta>();
            msg.fileMeta->fileId = fileJ.at("fileId").get<std::string>();
            msg.fileMeta->fileName = fileJ.at("fileName").get<std::string>();
            msg.fileMeta->mimeType = fileJ.at("mimeType").get<std::string>();
            msg.fileMeta->localPath = fileJ.at("localPath").get<std::string>();
            msg.fileMeta->fileSize = fileJ.at("fileSize").get<uint64_t>();
            msg.fileMeta->chunkIndex = fileJ.at("chunkIndex").get<uint32_t>();
            msg.fileMeta->totalChunks = fileJ.at("totalChunks").get<uint32_t>();
        } else {
            msg.fileMeta = nullptr;
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

SerializedMessage MiniEventAdapterImpl::serializedFileChunk(const FileMeta& chunk){
    nlohmann::json j;
    j["fileId"] = chunk.fileId;
    j["fileName"] = chunk.fileName;
    j["mimeType"] = chunk.mimeType;
    j["localPath"] = chunk.localPath;
    j["fileSize"] = chunk.fileSize;
    j["chunkIndex"] = chunk.chunkIndex;
    j["totalChunks"] = chunk.totalChunks;

    std::string jsonStr = j.dump();
    std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());
    uint32_t length = htonl(static_cast<uint32_t>(payload.size()));
    SerializedMessage result;
    result.push_back(0x02);  // 类型字节 0x02 for FileChunk
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&length), reinterpret_cast<uint8_t*>(&length) + sizeof(length));
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

bool MiniEventAdapterImpl::deserializeFileChunk(const SerializedMessage& serialized, FileMeta& chunk){
    if (serialized.size() < 1 + sizeof(uint32_t)) {
        return false;  // 不足以读取类型和长度
    }
    if (serialized[0] != 0x02) {
        return false;  // 类型不匹配
    }
    uint32_t length;
    memcpy(&length, &serialized[1], sizeof(length));
    length = ntohl(length);
    if (serialized.size() < 1 + sizeof(uint32_t) + length) {
        return false;  // 数据不足
    }
    std::string jsonStr(reinterpret_cast<const char*>(&serialized[1 + sizeof(uint32_t)]), length);
    try {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        chunk.fileId = j.at("fileId").get<std::string>();
        chunk.fileName = j.at("fileName").get<std::string>();
        chunk.mimeType = j.at("mimeType").get<std::string>();
        chunk.localPath = j.at("localPath").get<std::string>();
        chunk.fileSize = j.at("fileSize").get<uint64_t>();
        chunk.chunkIndex = j.at("chunkIndex").get<uint32_t>();
        chunk.totalChunks = j.at("totalChunks").get<uint32_t>();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

SerializedMessage MiniEventAdapterImpl::serializedLoginRequest(const LoginRequest& req){
    nlohmann::json j;
    j["username"] = req.username;
    j["password"] = req.password;

    std::string jsonStr = j.dump();
    std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());
    uint32_t length = htonl(static_cast<uint32_t>(payload.size()));
    SerializedMessage result;
    result.push_back(0x04);  // 类型字节 0x04 for LoginRequest
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&length), reinterpret_cast<uint8_t*>(&length) + sizeof(length));
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

bool MiniEventAdapterImpl::deserializeLoginResponse(const SerializedMessage& serialized, LoginResponse& resp){
    if (serialized.size() < 1 + sizeof(uint32_t)) {
        return false;  // 不足以读取类型和长度
    }
    if (serialized[0] != 0x03) {
        return false;  // 类型不匹配
    }
    uint32_t length;
    memcpy(&length, &serialized[1], sizeof(length));
    length = ntohl(length);
    if (serialized.size() < 1 + sizeof(uint32_t) + length) {
        return false;  // 数据不足
    }
    std::string jsonStr(reinterpret_cast<const char*>(&serialized[1 + sizeof(uint32_t)]), length);
    try {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        resp.success = j.at("success").get<bool>();
        resp.message = j.at("message").get<std::string>();
        resp.userId = j.at("userId").get<std::string>();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}