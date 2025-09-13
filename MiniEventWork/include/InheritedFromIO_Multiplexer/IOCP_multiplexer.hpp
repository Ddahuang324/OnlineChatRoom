#pragma once
#include "../IO_Multiplexer.hpp"
#include <vector>
#include <map>

#ifdef _WIN32
#include <windows.h>

class Channel;

class IOCPMultiplexer : public IOMultiplexer {
public:
    IOCPMultiplexer();
    ~IOCPMultiplexer() override;

    void addChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
    void updateChannel(Channel* channel) override;
    int dispatch(int timeout_ms, std::vector<Channel*>& active_channels) override;

private:
    HANDLE iocp_handle_;
    std::map<int, Channel*> channels_;
};

#endif
