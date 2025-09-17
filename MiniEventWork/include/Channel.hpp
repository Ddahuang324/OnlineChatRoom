#pragma once

#include <functional>
#include <memory>
#include <cstdint>

namespace MiniEventWork {

    class EventBase;
    class Channel : public std::enable_shared_from_this<Channel> {
public:

    using EventCallback = std::function<void()>;

    // 统一事件回调设置
    void setEventCallback(EventCallback cb) { event_callback_ = std::move(cb); }
        EventCallback getEventCallback() const;

    //事件掩码常量
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    static const int kErrorEvent;

    Channel(EventBase* loop, int fd);
    Channel(EventBase* loop); // for timer, fd = -1
    ~Channel();
    
    //核心：当事件发生时，有EventLoop调用
    void handleEvent();

     // 设置不同事件的回调函数
    void setReadCallback(EventCallback cb) { read_callback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { write_callback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { error_callback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { close_callback_ = std::move(cb); }
    // 清除所有回调，供拥有者在销毁时调用以防止悬空指针触发回调
    void clearCallbacks() { read_callback_ = EventCallback(); write_callback_ = EventCallback(); error_callback_ = EventCallback(); close_callback_ = EventCallback(); event_callback_ = EventCallback(); timer_callback_ = EventCallback(); }
    // ================== [新增] 定时器相关 ==================
    void setTimerCallback(EventCallback cb) { timer_callback_ = std::move(cb); }
    EventCallback getTimerCallback() const { return timer_callback_; }
    // =======================================================

    int fd( )const { return fd_; }
    int events( )const { return events_; }
    void ReadyEvents(int ready_events) { ready_events_ = ready_events; }
    int getReadyEvents() const { return ready_events_; }
    bool isNoneEvent( ) const { return events_ == kNoneEvent; }

//注册事件用的
    void enableReading( ) ; // 告诉系统监听fd上的读事件
    void disableReading( ) ; // 停止监听读事件
    void enableWriting( ) ; // 告诉系统开始监听这个fd上的写事件
    void disableWriting( ) ; //停止监听写事件
    void disableAll( ) ; // 停止监听所有事件

    bool isWriting( ) const { return events_ & kWriteEvent; }//检查当前是否启用了写事件监听
    bool isReading( ) const { return events_ & kReadEvent; }//检查当前是否启用了读事件监听
    bool isError( ) const { return events_ & kErrorEvent; }//检查当前是否启用了错误事件监听

    EventBase* ownerLoop( ) const { return loop_; }

    // ================== [新增] 定时器相关 ==================
    // 获取绝对超时时间（毫秒）
    uint64_t getTimeout() const { return timeout_; }
    void setTimeout(uint64_t timeout) { timeout_ = timeout; }
    
    // 获取/设置在最小堆中的索引
    int getHeapIndex() const { return heap_index_; }
    void setHeapIndex(int index) { heap_index_ = index; }
    // =======================================================

private:
    void update(); // 更新事件掩码

    EventBase* loop_;
    int fd_;

    int events_;
    int ready_events_; // 已经准备好的事件掩码

    // 回调函数
    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback error_callback_;
    EventCallback close_callback_; // 用于连接关闭的回调
    EventCallback event_callback_; // 统一事件回调

    // ================== [新增] 定时器相关 ==================
    EventCallback timer_callback_;   // 定时器回调
    uint64_t timeout_;              // 绝对超时时间戳
    int heap_index_;                // 在最小堆中的索引
    // =======================================================
};

} // namespace MiniEventWork


