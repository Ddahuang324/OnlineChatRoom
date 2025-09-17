#include "../include/EVConnConnector.hpp"
#include "../include/Channel.hpp"
#include "../include/EventBase.hpp"
#include "MiniEventLog.hpp"
#include "Platform.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

namespace {
// Helper function to set a socket to non-blocking mode.
bool setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) >= 0;
}
} // namespace

namespace MiniEventWork {

EVConnConnector::EVConnConnector(EventBase* loop, ConnectionCallback&& connection_callback)
    : loop_(loop),
      connection_callback_(std::move(connection_callback)),
      connecting_(false),
      retry_delay_ms_(500) // initial retry delay
{
    log_debug("EVConnConnector created.");
}

EVConnConnector::EVConnConnector(EventBase* loop, const ConnectionParams& params, ConnectionCallback&& connection_callback)
    : loop_(loop),
      connection_callback_(std::move(connection_callback)),
      connecting_(false),
      host_(params.host),
      port_(params.port),
      retry_delay_ms_(500) // initial retry delay
{
    log_debug("EVConnConnector created with params: host=%s, port=%d", host_.c_str(), port_);
    // 自动发起连接
    connect(host_, port_);
}

EVConnConnector::~EVConnConnector()
{
    log_debug("EVConnConnector destroyed.");
}

int EVConnConnector::connect(const std::string& host, int port) {
    if (connecting_) {
        log_warn("Already connecting.");
        return -1;
    }

    host_ = host;
    port_ = port;

    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_error("Failed to create socket.");
        return -1;
    }

    if (!setNonBlock(sockfd)) {
        log_error("Failed to set socket to non-blocking.");
        ::close(sockfd);
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        log_error("Invalid host address: %s", host.c_str());
        ::close(sockfd);
        return -1;
    }

    int ret = ::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == 0) {
        // Connected immediately
        connecting_ = false;
        if (connection_callback_) {
            connection_callback_(true, sockfd);
        }
        return 0;
    } else if (ret < 0 && errno == EINPROGRESS) {
        // Connecting asynchronously
        connecting_ = true;
        channel_ = std::make_unique<Channel>(loop_, sockfd);
        channel_->setWriteCallback([this] { this->handleConnect(); });
        channel_->enableWriting();
        return 0;
    } else {
        log_error("Connect failed immediately.");
        ::close(sockfd);
        return -1;
    }
}

void EVConnConnector::handleConnect() {
    int sockfd = channel_->fd();
    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
        err = errno;
    }

    if (err) {
        log_error("Connect failed: %s", strerror(err));
        ::close(sockfd);
        if (connection_callback_) {
            connection_callback_(false, -1);
        }
    } else {
        log_debug("Connected successfully.");
        channel_->disableAll();
        if (connection_callback_) {
            connection_callback_(true, sockfd);
        }
    }
    connecting_ = false;
}

void EVConnConnector::retry(int sockfd) {
    // For future use if needed
}

void EVConnConnector::removeChannel() {
    if (channel_) {
        loop_->removeChannel(channel_.get());
        channel_.reset();
    }
}

} // namespace MiniEventWork

