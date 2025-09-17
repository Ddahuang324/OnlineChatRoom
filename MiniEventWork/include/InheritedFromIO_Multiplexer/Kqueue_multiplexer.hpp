#pragma once
#include "../IO_Multiplexer.hpp"
#include <vector>
#include <map>

#ifdef __APPLE__
#include <sys/event.h>
#include <unistd.h>

namespace MiniEventWork {

class Channel;

class KqueueMultiplexer : public IOMultiplexer {
public:
    KqueueMultiplexer();
    ~KqueueMultiplexer() override;

    void addChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
    void updateChannel(Channel* channel) override;
    int dispatch(int timeout_ms, std::vector<Channel*>& active_channels) override;

private:
    void update(int operation, Channel* channel);
    static const int KInitEventListSize = 16;

    int kq_fd_;
    std::vector<struct kevent> events_;
    std::map<int, Channel*> channels_;
};

} // namespace MiniEventWork

#endif
