#include "../include/AbstractSever.hpp"

namespace MiniEventWork {

AbstractServer::AbstractServer(void)
    : msgHandler_(nullptr) {
}

AbstractServer::~AbstractServer(void) {
    delete msgHandler_;
}


void AbstractServer::run(struct EventBase* base ) {
    if( !msgHandler_ ) {
        msgHandler_ = createMsgHandler();
    }
    // 注意: 这里应该调用具体的 listen(port) 方法
    // run 方法的逻辑可能需要重新设计
}

MessageHandler* AbstractServer::createMsgHandler() {
    // 默认实现返回空指针，派生类应覆盖此方法以提供具体的消息处理器实例
    return nullptr;
}

// AbstractServer 不提供默认的 listen 实现，因为它是纯虚函数
// 子类必须实现自己的 listen(int port) 方法

} // namespace MiniEventWork
