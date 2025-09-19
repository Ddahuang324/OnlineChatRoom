#pragma once

#include "../../include/Buffer/Buffer.hpp"
#include <memory>
#include <functional>

namespace MiniEventWork {

class EventBase;
class Channel;

class BufferEvent : public std::enable_shared_from_this<BufferEvent> {
public:
    using Ptr = std::shared_ptr<BufferEvent>;
    using EventCallback = std::function<void(const Ptr)>;
    using ReadCallback = std::function<void(const BufferEvent::Ptr& , Buffer*)>;

    enum EventType {
        kReadEvent = 0x01,
        kWriteEvent = 0x02,
        kErrorEvent = 0x04,
        kEOFEvent = 0x08
    };

    BufferEvent(EventBase* loop, int fd);
    // 新增：从已有 channel 接管构造
    BufferEvent(EventBase* loop, std::unique_ptr<Channel> channel);
    ~BufferEvent();

    void setReadCallback(const ReadCallback& cb) { readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb) { writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb) { errorCallback_ = cb; }
    void setCloseCallback(const EventCallback& cb) { closeCallback_ = cb; }

    // Convenience helper: set multiple callbacks at once.
    // readCb: simple no-arg read handler
    // writeCb: simple no-arg write handler
    // eventCb: short event handler (uses EventType values)
    static void setCallBacks(const Ptr& bev,
                             const std::function<void()>& readCb,
                             const std::function<void()>& writeCb,
                             const std::function<void(short)>& eventCb);

    // Convenience: enable/disable read/write events on the underlying Channel.
    // Accepts either libevent-style flags (EV_READ/EV_WRITE) if available,
    // or the internal EventType flags (kReadEvent/kWriteEvent).
    static void enable(const Ptr& bev, int events);

    int fd() const { return fd_; }
    EventBase* getLoop() const { return loop_; }

    // Add method to access input buffer for reading
    Buffer& getInputBuffer() { return inputBuffer_; }

    // Extract next line from input buffer (up to \r\n), trim \r\n
    std::string trimLine();

    void connectEstablished();
    void write(const void* data, size_t len);
    void shutdown();
    void enableWriting(); // 启用写事件监听

private:
    void handleRead();
    void handleWrite();
    void handleError();
    void handleClose();
    void forceClose();

    EventBase* loop_;
    int fd_;
    std::unique_ptr<Channel> channel_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
    // 回调函数
    ReadCallback readCallback_;
    EventCallback writeCallback_; // output buffer清空时回调
    EventCallback closeCallback_; // 连接关闭时回调
    EventCallback errorCallback_;
};

} // namespace MiniEventWork
