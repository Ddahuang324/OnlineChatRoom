#include "../include/EVConnListener.hpp"
#include "MiniEventLog.hpp" // For logging
#include "Platform.hpp"     // For socket operations and types

#include <unistd.h>
#include <fcntl.h> // For fcntl

namespace {
// Helper function to set a socket to non-blocking mode.
// This is CRITICAL for an event-driven server.
bool setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) >= 0;
}
} // namespace

namespace MiniEventWork {

EVConnListener::EVConnListener(EventBase* loop, NewConnectionCallback&& new_connection_callback)
        : loop_(loop),
            new_connection_callback_(std::move(new_connection_callback)),
            listening_(false)
{
    // Initially, the channel has no valid FD.
        channel_ = std::make_unique<Channel>(loop_, -1);

        // Bind the internal handleAccept method to the channel's read callback.
        channel_->setReadCallback([this] { this->handleAccept(); });

        log_debug("EVConnListener created.");
}

EVConnListener::~EVConnListener()
{
    log_debug("EVConnListener destroyed.");
    // The unique_ptr for channel_ will handle its own destruction.
    // The listening socket FD is managed by the channel, which closes it.
}

int EVConnListener::listen(int port) {
    if (listening_) {
    log_warn("Listener is already listening.");
        return false;
    }

    int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
    log_error("Failed to create socket.");
        return false;
    }

    // Set socket options: non-blocking and reuse address.
    if (!setNonBlock(listenFd)) {
    log_error("Failed to set socket to non-blocking.");
        ::close(listenFd);
        return false;
    }
    int opt = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    log_error("Failed to bind socket.");
        ::close(listenFd);
        return -1;
    }

    if (::listen(listenFd, SOMAXCONN) < 0) {
    log_error("Failed to listen on socket.");
        ::close(listenFd);
        return -1;
    }

    // Update the channel with the new, valid listening socket FD.
    // Channel currently doesn't expose setFd in the header; instead we created it with -1
    // and rely on EventBase::updateChannel when enabling reading. If Channel had a setter
    // for fd, it should be used here. For now, assume Channel::enableReading will register
    // the channel based on its fd() (we created with -1). To be correct, recreate channel
    // with the valid fd.
    channel_ = std::make_unique<Channel>(loop_, listenFd);
    channel_->setReadCallback([this] { this->handleAccept(); });
    channel_->enableReading(); // This internally calls updateChannel in EventBase.

    listening_ = true;
    log_info("Server listening on port %d", port);
    return 0;
}

int EVConnListener::getfd() const {
    return channel_->fd();
}

void EVConnListener::handleAccept() {
    struct sockaddr_in peerAddr;
    socklen_t len = sizeof(peerAddr);

    // The core optimization: Loop to accept all pending connections.
    while (true) {
        int connFd = ::accept(channel_->fd(), (struct sockaddr*)&peerAddr, &len);

        if (connFd >= 0) {
            // Success! We have a new connection.
            log_debug("Accepted new connection with fd = %d", connFd);
            if (new_connection_callback_) {
                new_connection_callback_(connFd, (struct sockaddr*)&peerAddr, len);
            }
        } else {
            // An error occurred.
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // This is expected for a non-blocking socket when no more connections are pending.
                // We've processed all incoming connections for now.
                log_debug("All pending connections have been accepted.");
                break;
            } else if (errno == EMFILE) {
                // Process ran out of file descriptors. A serious issue.
                // A robust server might close the connection gracefully or have other strategies.
                log_error("Could not accept new connection: Too many open files (EMFILE).");
                // Here, we just log and break.
                break;
            } else {
                log_error("Error in accept(): %s", strerror(errno));
                break;
            }
        }
    }
}

} // namespace MiniEventWork
