#pragma once

#include "net/MiniEventAdapter.h"
#include "net/IMiniEventAdapter.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <atomic>
#include <deque>
#include <chrono>
#include <vector>
#include <cstdint>

// 这里只需要前向声明 BufferEvent (若将来需要)；EventBase 已在接口头中以命名空间前向声明
namespace MiniEventWork {
    class BufferEvent; // 如果后续需要指针或引用
    class EventBase;
    class EVConnConnector;
}

using SerializedMessage = std::vector<uint8_t>;

class MiniEventAdapterImpl : public MiniEventAdapter, public IMiniEventAdapter {
public:
    MiniEventAdapterImpl();
    ~MiniEventAdapterImpl() override;


    void init(MiniEventWork::EventBase* eventBase,
              ConnectionCallback connCb,
              MessageCallback msgCb,
              FileChunkCallback fileCb,
              LoginCallback loginCb) override;

    void connect(const ConnectionParams& params) override;
    void disconnect() override;
    void sendMessage(const MessageRecord& msg) override;
    void sendFileChunk(const FileMeta& chunk) override;
    void sendLoginRequest(const LoginRequest& req) override;


private:
    // 不拥有的指针（若 isInternalEventBase_ 为 true 则由本类负责生命周期）
    MiniEventWork::EventBase* eventBase_{nullptr};
    std::unique_ptr<std::thread> eventThread_;
    std::atomic<bool> isInternalEventBase_{false};
    std::deque<SerializedMessage> sendQueue_;
    std::mutex sendQueueMutex_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    FileChunkCallback fileCallback_;
    LoginCallback loginCallback_;


    std::shared_ptr<MiniEventWork::BufferEvent> bev_{nullptr}; // 使用 shared_ptr 管理 BufferEvent
    // 将裸指针改为带自定义删除器的 unique_ptr，删除器会在 EventBase 线程上执行删除（如果可能）
    std::unique_ptr<MiniEventWork::EVConnConnector, std::function<void(MiniEventWork::EVConnConnector*)>> connector_{nullptr};

    // Reconnection related
    std::atomic<bool> reconnecting_{false};
    int retryCount_{0};
    std::chrono::milliseconds backoffTime_{1000};
    ConnectionParams lastParams_;

    // Callback handlers
    void onRead();
    void onEvent(short events);
    //sendmessage辅助函数
    void processSendQueue();
    void attemptReconnect();

    SerializedMessage serializedMessage(const MessageRecord msg); // 需要实现序列化函数
    bool deserializeMessage(const SerializedMessage& serialized, MessageRecord& msg); // 反序列化函数
    SerializedMessage serializedFileChunk(const FileMeta& chunk);
    bool deserializeFileChunk(const SerializedMessage& serialized, FileMeta& chunk);
    SerializedMessage serializedLoginRequest(const LoginRequest& req);
    bool deserializeLoginResponse(const SerializedMessage& serialized, LoginResponse& resp);
};