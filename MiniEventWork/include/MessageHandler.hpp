#pragma once 

namespace MiniEventWork {

class AbstractServer;

class MessageHandler {
public:

    MessageHandler(void) = default;
    virtual ~MessageHandler(void)= default;

    virtual void handleMessage(void* arg) = 0  ;

};

} // namespace MiniEventWork

