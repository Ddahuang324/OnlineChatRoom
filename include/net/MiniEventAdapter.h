#pragma once

// Placeholder for the MiniEventAdapter abstract interface as per T021.
// Enum: NetStatus { Ok, NotConnected, Timeout, QueueFull, SerializeError, InternalError }
// Implementation will be provided by the user.
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

using SerializedMessage = std::vector<uint8_t>;


enum class NetStatus{
    OK,
    NOT_CONNECTED,
    TIMEOUT,
    QUEUE_FULL,
    SERIALIZATION_ERROR,
    INTERNAL_ERROR,
};

class MiniEventAdapter {

public:
    virtual ~MiniEventAdapter() = default;

    // 连接成功时调用的回调函数
    std::function<void()> onConnected;
    // 断开连接时调用的回调函数
    std::function<void()> onDisconnected;
    // 接收到消息时调用的回调函数
    std::function<void(const SerializedMessage&)> onMessageReceived;
    // 发送文件块成功时调用的回调函数
    std::function<void(uint64_t fileid, uint32_t chunkindex, NetStatus status)> onFileChunkSent;

    // 连接到指定的IP地址和端口
    virtual void connect(const std::string& ip, uint16_t port) = 0;
    // 断开当前连接
    virtual void disconnect() = 0;

    // 发送消息
    virtual NetStatus send(const SerializedMessage& message) = 0;

    // 发送文件块
    virtual NetStatus sendFileChunk(uint64_t fileid, uint32_t chunkindex,const std::vector<uint8_t>& data) = 0;


};