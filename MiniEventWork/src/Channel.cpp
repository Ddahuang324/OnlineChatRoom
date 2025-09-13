#include "../include/Channel.hpp"
#include "../include/EventBase.hpp"
#include "../include/MiniEventLog.hpp"

#include <exception>

// --- 静态常量成员的定义 ---
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = 1;
const int Channel::kWriteEvent = 2;
const int Channel::kErrorEvent = 4;

// --- 构造函数 ---
Channel::Channel(EventBase* loop, int fd)
    : loop_(loop), fd_(fd), events_(0), ready_events_(0), 
      timeout_(0), heap_index_(-1) {
}

// --- 析构函数 ---
Channel::~Channel(){
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

Channel::EventCallback Channel::getEventCallback() const { return event_callback_; }

// --- 处理事件 ---
void Channel::handleEvent(){
    // 保护性检查：确保该 channel 仍然由 EventBase 管理且 fd 有效
    if (!loop_ || !loop_->hasChannel(this)) {
        log_warn("Channel::handleEvent called for fd=%d but channel not registered in loop, skipping.", fd_);
        return;
    }

    if (fd_ < 0) {
        log_warn("Channel::handleEvent called for invalid fd=%d, skipping.", fd_);
        return;
    }

    // 调试输出当前就绪事件情况
    std::cerr << "[Channel] handleEvent fd=" << fd_ 
              << " ready_events=" << ready_events_ 
              << " events(mask)=" << events_
              << " has(event)=" << (event_callback_?1:0)
              << " has(read)=" << (read_callback_?1:0)
              << " has(write)=" << (write_callback_?1:0)
              << " has(error)=" << (error_callback_?1:0)
              << std::endl;

    // 如果设置了统一事件回调（例如 BufferEvent 绑定的 handleEvent），优先调用并直接返回
    if(event_callback_){
        // Protect against the callback destroying this Channel during execution.
        std::shared_ptr<Channel> guard;
        try {
            guard = shared_from_this();
        } catch (...) {
            // If we can't create shared ptr, proceed without guard but still call safely.
        }
        try {
            event_callback_();
        } catch (const std::exception &ex) {
            log_error("Exception in event_callback for fd=%d: %s", fd_, ex.what());
        } catch (...) {
            log_error("Unknown exception in event_callback for fd=%d", fd_);
        }
        return;
    }
    // 否则按事件类型分别回调
    if(ready_events_ & kReadEvent){
        if(read_callback_ != nullptr) {
            std::shared_ptr<Channel> guard;
            try { guard = shared_from_this(); } catch(...) {}
            try {
                std::cerr << "[Channel] invoking read_callback fd=" << fd_ << std::endl;
                read_callback_();
                std::cerr << "[Channel] read_callback returned fd=" << fd_ << std::endl;
            } catch (const std::exception &ex) {
                log_error("Exception in read_callback for fd=%d: %s", fd_, ex.what());
            } catch (...) {
                log_error("Unknown exception in read_callback for fd=%d", fd_);
            }
        }
    }
    if(ready_events_ & kWriteEvent){
        if(write_callback_ != nullptr) {
            std::shared_ptr<Channel> guard;
            try { guard = shared_from_this(); } catch(...) {}
            try {
                std::cerr << "[Channel] invoking write_callback fd=" << fd_ << std::endl;
                write_callback_();
                std::cerr << "[Channel] write_callback returned fd=" << fd_ << std::endl;
            } catch (const std::exception &ex) {
                log_error("Exception in write_callback for fd=%d: %s", fd_, ex.what());
            } catch (...) {
                log_error("Unknown exception in write_callback for fd=%d", fd_);
            }
        }
    }
    if(ready_events_ & kErrorEvent){
        if(error_callback_ != nullptr) {
            std::shared_ptr<Channel> guard;
            try { guard = shared_from_this(); } catch(...) {}
            try {
                std::cerr << "[Channel] invoking error_callback fd=" << fd_ << std::endl;
                error_callback_();
                std::cerr << "[Channel] error_callback returned fd=" << fd_ << std::endl;
            } catch (const std::exception &ex) {
                log_error("Exception in error_callback for fd=%d: %s", fd_, ex.what());
            } catch (...) {
                log_error("Unknown exception in error_callback for fd=%d", fd_);
            }
        }
    }
}

void Channel::enableReading(){
    events_ |= kReadEvent;
    update();
}

void Channel::enableWriting(){
    events_ |= kWriteEvent;
    update();
}

void Channel::disableWriting() {
    events_ &= ~kWriteEvent; // 使用位反和与运算，将"写"标志位清零
    update();
}

void Channel::disableReading() {
    events_ &= ~kReadEvent; // 使用位反和与运算，将"读"标志位清零
    update();
}

void Channel::disableAll() {
    events_ = kNoneEvent;
    update();
}

// --- 私有辅助函数 ---
void Channel::update() {
    loop_->updateChannel(this);
}