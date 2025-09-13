#pragma once

#include <vector>
#include <map>

class Channel;//将原来的fd，event等等都封装到这里面


class IOMultiplexer {
public:
    virtual ~IOMultiplexer() = default;

    //核心接口
    virtual void addChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual int dispatch(int timeout_ms,std::vector<Channel*>& active_channels) = 0;

    
protected:
    IOMultiplexer() = default;
};





