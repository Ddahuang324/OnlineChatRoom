#pragma once

#include "../include/MessageHandler.hpp"

namespace MiniEventWork {

struct EventBase;

class AbstractServer {
public:
    AbstractServer(void);
    virtual ~AbstractServer(void);


    void run(struct EventBase* base );

    virtual int listen(int port) = 0;
    
    virtual MessageHandler* createMsgHandler() = 0;

    MessageHandler* getMsgHandler() const { return msgHandler_; }

private:
    MessageHandler* msgHandler_;
};

} // namespace MiniEventWork
