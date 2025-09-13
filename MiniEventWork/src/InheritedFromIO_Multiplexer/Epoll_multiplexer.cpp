#ifdef __linux__

#include "../../include/InheritedFromIO_Multiplexer/Epoll_multiplexer.hpp"
#include "../../include/Channel.hpp"
#include "../../include/MiniEventLog.hpp" 
#include <cassert>
#include <cerrno>

EpollMultiplexer::EpollMultiplexer() : 
    epoll_fd_(epoll_create1(EPOLL_CLOEXEC)) ,
    events_(KInitEventListSize) {
        if (epoll_fd_ < 0)  {log_error("[Epoll] epoll_create1 error");}
        else { std::cout << "[Epoll] created" << std::endl; }
    }


EpollMultiplexer::~EpollMultiplexer() {
    close(epoll_fd_);
    std::cout << "[Epoll] destroyed" << std::endl;
}

void EpollMultiplexer::addChannel(Channel* channel) {
    update (EPOLL_CTL_ADD, channel);
    channels_[channel->fd()] = channel;
}

void EpollMultiplexer::removeChannel(Channel* channel) {
    channels_.erase(channel->fd());
    update(EPOLL_CTL_DEL, channel);
}

void EpollMultiplexer::updateChannel(Channel* channel) {
    update(EPOLL_CTL_MOD, channel);
}

void EpollMultiplexer::update(int operation, Channel* channel) {
    struct epoll_event event; 
    event.events = channel->events();
    event.data.ptr = channel;//将channel的指针作为data.ptr
    event.data.fd = channel->fd();
    if (epoll_ctl(epoll_fd_, operation, channel->fd(), &event) < 0) {
        log_error("[Epoll] epoll_ctl error");
    }

}

int EpollMultiplexer::dispatch(int timeout_ms,std::vector<Channel*>& active_channels) {
    int num_events = epoll_wait(epoll_fd_, &*events_.begin(), static_cast<int>(events_.size()), timeout_ms);

    int saved_errno = errno;

    if (num_events > 0) {
        for (int i = 0; i < num_events; ++i) {
            Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->ReadyEvents(events_[i].events);
            active_channels.push_back(channel);
        }
        if (static_cast<size_t>(num_events) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    }else if (num_events == 0) {

    }else{
        if (saved_errno != EINTR) {
            errno = saved_errno;
            log_error("[Epoll] epoll_wait error");
        }
    }
    return num_events;
}


#endif
