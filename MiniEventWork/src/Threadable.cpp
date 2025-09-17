#include "../include/MultiThread/Threadable.hpp"

namespace MiniEventWork {

Threadable::Threadable() : threadId_(0), isRunning_(false) {

}

Threadable::~Threadable() {
    if( isRunning_ ) {
        pthread_detach(threadId_);// 分离线程，避免僵尸线程
    }
}


bool Threadable::start() {
    if( isRunning_) return false;// 已经在运行
    if( pthread_create(&threadId_, nullptr, thread_proxy, this) != 0 ) {
        return false;
    }//创建线程失败
    isRunning_ = true;
    return true;
}



void Threadable::join() {
    if( isRunning_ ) {
        pthread_join(threadId_, nullptr);
        isRunning_ = false;
    }
    threadId_ = 0;
    isRunning_ = false;
}

pthread_t Threadable::getThreadId() const {
    return threadId_;
}

void* Threadable::thread_proxy(void* args) {
    Threadable* thread = static_cast<Threadable*>(args);
    thread->run();
    return nullptr;
}

} // namespace MiniEventWork

