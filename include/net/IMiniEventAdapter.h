#pragma once

#include <QObject>
#include "net/MiniEventAdapter.h"
#include "model/Entities.hpp"

class IMiniEventAdapter : public QObject {
    Q_OBJECT

public:
    virtual ~IMiniEventAdapter() = default;

    public:
    virtual void connect(const ConnectionParams& params) = 0;
    virtual void sendLoginRequest(const LoginRequest& req) = 0;
    virtual void sendMessage(const MessageRecord& msg) = 0;

signals:
    void connectionEvent(connectionEvents event);
    void loginResponse(const LoginResponse& resp);
    void messageReceived(const MessageRecord& msg);
};