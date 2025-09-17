#pragma once

#include <functional>
#include <memory>
#include "../include/Channel.hpp"
#include "../include/EventBase.hpp"

namespace MiniEventWork {

struct sockaddr;

class EVConnListener
{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const struct sockaddr* addr,socklen_t len)>;
    EVConnListener(EventBase* loop, NewConnectionCallback&& new_connection_callback);
    ~EVConnListener();

    EVConnListener(const EVConnListener&) = delete;
    EVConnListener& operator=(const EVConnListener&) = delete;

    int listen(int port);

    int getfd() const;

private:
    void handleAccept();
    std::unique_ptr<Channel> channel_;
    EventBase* loop_;
    NewConnectionCallback new_connection_callback_;
    bool listening_;
};

} // namespace MiniEventWork
