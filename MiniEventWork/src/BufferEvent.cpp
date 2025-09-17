#include "../include/Buffer/BufferEvent.hpp"
#include "../include/EventBase.hpp"
#include "../include/Channel.hpp" // Ensure the full definition of Channel is included
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>

namespace MiniEventWork {

BufferEvent::BufferEvent(EventBase* loop, int fd)
    : loop_(loop),
      fd_(fd),
      channel_(new Channel(loop, fd))
    {
  // 注册各类事件回调，避免在 BufferEvent 内再次做事件类型分发
  channel_->setReadCallback(std::bind(&BufferEvent::handleRead, this));
  channel_->setWriteCallback(std::bind(&BufferEvent::handleWrite, this));
  channel_->setErrorCallback(std::bind(&BufferEvent::handleError, this));
  channel_->setCloseCallback(std::bind(&BufferEvent::handleClose, this));
    }

BufferEvent::~BufferEvent() {
  // 在析构之前，先清除 channel 上的回调以避免被事件循环误调用
  if (channel_) {
    channel_->clearCallbacks();
  }
  // 不在析构阶段主动 removeChannel：生命周期应该由主动关闭流程控制
  // 防止已经在 handleClose 中移除后再调用 remove 引发断言
  // fd_ 的关闭交由 Channel 析构自动完成
}


void BufferEvent::connectEstablished() {
  //连接建立后，开始监听读事件
    channel_->enableReading();
}

// 事件分发已交由 Channel 调用各自回调，BufferEvent 不再实现统一的 handleEvent

void BufferEvent::handleRead() {
  int savedErrno = 0;

  ssize_t n = inputBuffer_.readFd(fd_, &savedErrno);
  if(n > 0){
    if(readCallback_){
      readCallback_(shared_from_this(), &inputBuffer_);
    }
  } else if (n == 0) {
    handleClose();
  } else {
    errno = savedErrno;
    handleError();
  }
}


void BufferEvent::handleWrite() {
  // 如果 channel 正在进行写操作
  if(channel_-> isWriting()){
    // 无数据 => 连接建立阶段，关闭写监听防止busy loop
    if(outputBuffer_.readableBytes() == 0){
      if(writeCallback_) writeCallback_(shared_from_this());
      channel_->disableWriting();
      return;
    }
    ssize_t n  = ::write(fd_, outputBuffer_.peek(), outputBuffer_.readableBytes());
    if(n > 0){
      outputBuffer_.retrieve(n);
      if(outputBuffer_.readableBytes() == 0){
        channel_->disableWriting();
        if(writeCallback_) writeCallback_(shared_from_this());
      }
    } else if(n < 0){
      handleError();
    }
  }
}

void BufferEvent::write(const void* data, size_t len) {
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 如果输出缓冲区为空，尝试直接发送
    if (outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(fd_, data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCallback_) {
                // 全部一次性发送完毕
                writeCallback_(shared_from_this());
            }
        } else { // nwrote < 0
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                faultError = true;
            }
        }
    }

    // 如果没有出错，且还有数据没发完，就放入输出缓冲区
    if (!faultError && remaining > 0) {
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        if (!channel_->isWriting()) {
            channel_->enableWriting(); // 开始监听可写事件
        }
    }
}

void BufferEvent::handleClose() {
  // 停止所有事件监听并清除回调，随后调用外部 closeCallback
  channel_->disableAll(); // 停止所有事件监听
  // 从 EventBase 中移除（防止后续再触发 updateChannel 导致断言）
  if (loop_->hasChannel(channel_.get())) {
    loop_->removeChannel(channel_.get());
  }
  // 先交换回调，防止回调中再次触发删除导致双重调用
  auto cb = closeCallback_;
  closeCallback_ = nullptr;
  if (cb) {
    cb(shared_from_this());
  }
}

void BufferEvent::handleError() {
    channel_->disableAll(); // 停止所有事件监听
    if (errorCallback_) {
        errorCallback_(shared_from_this());
    }
}

void BufferEvent::shutdown() {
    // 关闭写端，触发关闭事件
    ::shutdown(fd_, SHUT_WR);
    // 停止监听写事件
    channel_->disableWriting();
}

void BufferEvent::enableWriting() {
    // 启用写事件监听
    channel_->enableWriting();
}

// Extract next line from input buffer (up to \r\n), trim \r\n
std::string BufferEvent::trimLine() {
    size_t crlf_pos = inputBuffer_.findCRLF();
    if (crlf_pos == 0) return "";

    std::string line = inputBuffer_.retrieveAsString(crlf_pos);
    // Remove \r\n
    if (!line.empty() && line.back() == '\n') line.pop_back();
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}

// static helper
void BufferEvent::setCallBacks(const Ptr& bev,
                 const std::function<void()>& readCb,
                 const std::function<void()>& writeCb,
                 const std::function<void(short)>& eventCb) {
  if (!bev) return;

  if (readCb) {
    bev->setReadCallback([readCb](const BufferEvent::Ptr& self, Buffer* buf){
      // ignore params and call simple read callback
      readCb();
    });
  } else {
    bev->setReadCallback(ReadCallback());
  }

  if (writeCb) {
    bev->setWriteCallback([writeCb](const BufferEvent::Ptr& self){
      writeCb();
    });
  } else {
    bev->setWriteCallback(EventCallback());
  }

  if (eventCb) {
    bev->setErrorCallback([eventCb](const BufferEvent::Ptr& self){
      // BufferEvent currently does not pass event flags on error callback;
      // we call with 0 as placeholder. If needed, expand interface.
      eventCb(0);
    });
    bev->setCloseCallback([eventCb](const BufferEvent::Ptr& self){
      eventCb(BufferEvent::kEOFEvent);
    });
  } else {
    bev->setErrorCallback(EventCallback());
    bev->setCloseCallback(EventCallback());
  }
}

void BufferEvent::enable(const Ptr& bev, int events) {
  if (!bev || !bev->channel_) return;

  // Normalize events: accept either EV_READ/EV_WRITE from libevent or internal flags
#ifdef EV_READ
  const int ev_read = EV_READ;
  const int ev_write = EV_WRITE;
#else
  const int ev_read = kReadEvent;
  const int ev_write = kWriteEvent;
#endif

  if (events & ev_read) {
    bev->channel_->enableReading();
  }
  if (events & ev_write) {
    bev->channel_->enableWriting();
  }
}

} // namespace MiniEventWork