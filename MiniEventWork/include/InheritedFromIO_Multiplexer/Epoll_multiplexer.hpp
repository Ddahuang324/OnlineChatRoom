#pragma once
#include "../IO_Multiplexer.hpp"
#include <vector>


#ifdef __linux__
#include <sys/epoll.h>
#include <unistd.h>


class Channel;


class EpollMultiplexer : public IOMultiplexer {
public:
    EpollMultiplexer();
    ~EpollMultiplexer() override;

    void addChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
    void updateChannel(Channel* channel) override;
    int dispatch(int timeout_ms,std::vector<Channel*>& active_channels) override;

    //禁止拷贝
    EpollMultiplexer(const EpollMultiplexer&) = delete;
    EpollMultiplexer& operator=(const EpollMultiplexer&) = delete;

private:
    void update(int opration, Channel* channel);
    static const int KInitEventListSize = 16;

    int epoll_fd_;
    std::vector<struct epoll_event> events_;
    std::map<int, Channel*> channels_;
};


#endif