#ifdef _WIN32

#include "../../include/InheritedFromIO_Multiplexer/IOCP_multiplexer.hpp"
#include "../../include/Channel.hpp"
#include "../../include/MiniEventLog.hpp"

#include <windows.h>

IOCPMultiplexer::IOCPMultiplexer() : iocp_handle_(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) {
    if (iocp_handle_ == NULL) {
        log_error("[IOCP] CreateIoCompletionPort failed");
    } else {
        std::cout << "[IOCP] created" << std::endl;
    }
}

IOCPMultiplexer::~IOCPMultiplexer() {
    if (iocp_handle_) CloseHandle(iocp_handle_);
    std::cout << "[IOCP] destroyed" << std::endl;
}

void IOCPMultiplexer::addChannel(Channel* channel) {
    // Associate the socket handle with the IOCP. Here we only support sockets.
    HANDLE h = (HANDLE) (uintptr_t) channel->fd();
    if (CreateIoCompletionPort(h, iocp_handle_, (ULONG_PTR)channel, 0) == NULL) {
        log_error("[IOCP] CreateIoCompletionPort associate failed");
    }
    channels_[channel->fd()] = channel;
}

void IOCPMultiplexer::removeChannel(Channel* channel) {
    channels_.erase(channel->fd());
    // No direct deletion API for IOCP associations; sockets closed elsewhere.
}

void IOCPMultiplexer::updateChannel(Channel* channel) {
    // IOCP uses overlapped operations; updating interest is done by posting ops.
}

int IOCPMultiplexer::dispatch(int timeout_ms, std::vector<Channel*>& active_channels) {
    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped;
    BOOL res = GetQueuedCompletionStatus(iocp_handle_, &bytesTransferred, &completionKey, &overlapped, (DWORD)timeout_ms);
    if (!res) {
        DWORD err = GetLastError();
        if (err == WAIT_TIMEOUT) return 0;
        log_error("[IOCP] GetQueuedCompletionStatus failed");
        return -1;
    }
    Channel* ch = (Channel*)completionKey;
    if (ch) {
        // Mark as readable/writable based on bytesTransferred or overlapped specifics.
        ch->ReadyEvents(Channel::kReadEvent);
        active_channels.push_back(ch);
    }
    return 1;
}

#endif
