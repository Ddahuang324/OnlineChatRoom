#pragma once

#include "../../include/MessageHandler.hpp"

class TCPMsgHandler : public MessageHandler {
public:
    TCPMsgHandler() = default;
    virtual ~TCPMsgHandler() = default;

    virtual void handleMessage(void* arg) override;
};