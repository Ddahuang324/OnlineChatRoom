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
    if (connector_) {
        delete connector_;
        connector_ = nullptr;
    }
}

void MiniEventAdapterImpl::init(MiniEventWork::EventBase* eventBase,ConnectionCallback connCb,MessageCallback msgCb,FileChunkCallback fileCb,LoginCallback loginCb){
   
    connectionCallback_ = [this, connCb](connectionEvents event) { if (connCb) connCb(event); emit connectionEvent(event); };
    messageCallback_ = std::move(msgCb);
    fileCallback_ = std::move(fileCb);
    loginCallback_ = [this, loginCb](const LoginResponse& resp) { if (loginCb) loginCb(resp); emit loginResponse(resp); };


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
        connector_ = new MiniEventWork::EVConnConnector(eventBase_, params, [this](bool success, int sockfd) {
            if (success) {
                // 连接成功，先移除连接器的channel
                connector_->removeChannel();
                // 然后创建 BufferEvent
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