#pragma once

#include "../IO_Multiplexer.hpp"
#include <vector>
#include <map>


#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

namespace MiniEventWork {

class SelectMultiplexer : public IOMultiplexer {
public:
    SelectMultiplexer();
    ~SelectMultiplexer() ;

    void addChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
    void updateChannel(Channel* channel) override;
    int dispatch(int timeout_ms,std::vector<Channel*>& active_channels) override;

private:
    fd_set read_set_;
    fd_set write_set_;
    fd_set error_set_;
    int max_fd_;
    std::map<int, Channel*> channels_;
};

} // namespace MiniEventWork
