#include <pthread.h>

class Threadable {

public:
    Threadable();
    virtual ~Threadable();

    bool start();// 启动线程

    void join();// 等待线程结束

    pthread_t getThreadId() const;

protected:
    virtual void run() = 0;

private:
    static void* thread_proxy(void* args);
    pthread_t threadId_;
    bool isRunning_;
};