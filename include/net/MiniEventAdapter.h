#pragma once

#include <string>
#include <vector>
#include <functional>
#include "model/Entities.hpp"

namespace MiniEventWork {
    class EventBase;
}

struct ConnectionParams {
    std::string host;//服务器地址
    int port;//服务器端口
    //可拓展

    bool operator==(const ConnectionParams& other) const {
        return host == other.host && port == other.port;
    }
};

enum class connectionEvents {
    Connecting,
    Connected,
    Disconnected,
    ConnectFailed,
};

//序列化后对消息，用于适配器内部和传送队列中传递
using SerializedMessage = std::vector<uint8_t>; 

class MiniEventAdapter {
public:
    using ConnectionCallback = std::function<void(connectionEvents)>;
    using MessageCallback = std::function<void(const MessageRecord&)>;
    using FileChunkCallback = std::function<void(const FileMeta)>;
    using LoginCallback = std::function<void(const LoginResponse&)>;

    virtual ~MiniEventAdapter() = default;


     virtual void init(MiniEventWork::EventBase* eventBase,
                      ConnectionCallback connCb,
                      MessageCallback msgCb,
                      FileChunkCallback fileCb,
                      LoginCallback loginCb) = 0;


    virtual void connect(const ConnectionParams& params) = 0;

    virtual void disconnect() = 0;

   //virtual connectionEvents sendMessage(const MessageRecord& msg) = 0; 不能是 ConnectionEVents 事件 为什么？ 因为状态的发送的结果是异步的，未来才可能知道的

    //virtual connectionEvents sendFileChunk(const FileMeta& fileMeta, const std::vector<uint8_t>& chunk) = 0;

    virtual void sendMessage(const MessageRecord& msg) = 0;
    virtual void sendFileChunk(const FileMeta& chunk) = 0;
    virtual void sendLoginRequest(const LoginRequest& req) = 0;

};





