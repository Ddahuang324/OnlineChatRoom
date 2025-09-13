#ifdef __APPLE__

#include "../../include/InheritedFromIO_Multiplexer/Kqueue_multiplexer.hpp"
#include "../../include/Channel.hpp"
#include "../../include/MiniEventLog.hpp"

#include <cassert>
#include <cerrno>
#include <unistd.h>

KqueueMultiplexer::KqueueMultiplexer()
    : kq_fd_(kqueue()), events_(KInitEventListSize) {
    if (kq_fd_ < 0) { log_error("[Kqueue] create error"); }
    else { std::cout << "[Kqueue] created" << std::endl; }
}

KqueueMultiplexer::~KqueueMultiplexer() {
    close(kq_fd_);
    std::cout << "[Kqueue] destroyed" << std::endl;
    
}

void KqueueMultiplexer::addChannel(Channel* channel) {
    update(EV_ADD | EV_ENABLE, channel);
    channels_[channel->fd()] = channel;
}

void KqueueMultiplexer::removeChannel(Channel* channel) {
    channels_.erase(channel->fd());
    update(EV_DELETE, channel);
}

void KqueueMultiplexer::updateChannel(Channel* channel) {
    update(0, channel);
}

void KqueueMultiplexer::update(int operation, Channel* channel) {
    struct kevent kev[2];
    int n = 0;
    int fd = channel->fd();
    int events = channel->events();

    if (events & Channel::kReadEvent) {
        EV_SET(&kev[n++], fd, EVFILT_READ, operation ?: EV_ADD | EV_ENABLE, 0, 0, channel);
    }
    if (events & Channel::kWriteEvent) {
        EV_SET(&kev[n++], fd, EVFILT_WRITE, operation ?: EV_ADD | EV_ENABLE, 0, 0, channel);
    }

    if (n > 0) {
        if (kevent(kq_fd_, kev, n, nullptr, 0, nullptr) < 0) {
            log_error("[Kqueue] kevent register error");
        }
    }
}

int KqueueMultiplexer::dispatch(int timeout_ms, std::vector<Channel*>& active_channels) {
    struct timespec ts;
    struct timespec* tsp = nullptr;
    if (timeout_ms >= 0) {
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;
        tsp = &ts;
    }

    int num_events = kevent(kq_fd_, nullptr, 0, &*events_.begin(), static_cast<int>(events_.size()), tsp);

    if (num_events > 0) {
        for (int i = 0; i < num_events; ++i) {
            Channel* ch = static_cast<Channel*>(events_[i].udata);
            int revents = 0;
            if (events_[i].filter == EVFILT_READ) revents |= Channel::kReadEvent;
            if (events_[i].filter == EVFILT_WRITE) revents |= Channel::kWriteEvent;
            ch->ReadyEvents(revents);
            active_channels.push_back(ch);
        }
        if (static_cast<size_t>(num_events) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (num_events < 0) {
        if (errno != EINTR) {
            log_error("[Kqueue] kevent wait error");
        }
    }
    return num_events;
}

#endif
