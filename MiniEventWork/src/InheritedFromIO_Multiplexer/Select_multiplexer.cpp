#include "../../include/InheritedFromIO_Multiplexer/Select_multiplexer.hpp"
#include "../../include/Channel.hpp"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

namespace MiniEventWork {



SelectMultiplexer::SelectMultiplexer(): max_fd_(-1){
    FD_ZERO(&read_set_);
    FD_ZERO(&write_set_);
    FD_ZERO(&error_set_);
    std::cout << "[Select] created" << std::endl;
}

SelectMultiplexer::~SelectMultiplexer() {
    std::cout << "[Select] destroyed" << std::endl;
}

void SelectMultiplexer::addChannel(Channel* channel) {
    int fd = channel->fd();
    channels_[fd] = channel;
    
    // 不处理虚拟fd（负数或定时器专用）
    if (fd < 0) {
        return;
    }
    // 保护：避免越界 FD_SETSIZE
    if (fd >= FD_SETSIZE) {
        std::cerr << "[Select][warn] fd=" << fd
                  << " >= FD_SETSIZE=" << FD_SETSIZE
                  << ", skip tracking in select()" << std::endl;
        return;
    }
    
    if (fd > max_fd_) {
        max_fd_ = fd;
    }
    if (channel->isReading()) FD_SET(fd, &read_set_);
    if (channel->isWriting()) FD_SET(fd, &write_set_);
}

void SelectMultiplexer::removeChannel(Channel* channel) {
    int fd = channel->fd();

    // 不处理虚拟fd（负数或定时器专用）
    if (fd >= 0) {
        if (fd < FD_SETSIZE) {
            FD_CLR (fd, &read_set_);
            FD_CLR (fd, &write_set_);
            FD_CLR (fd, &error_set_);
        }
    }

    channels_.erase(fd);

    if (fd == max_fd_) {
        max_fd_ = -1;
        // 重新计算max_fd_，只考虑有效的fd
        for (auto& pair : channels_) {
            if (pair.first >= 0 && pair.first > max_fd_) {
                max_fd_ = pair.first;
            }
        }
    }
}


void SelectMultiplexer::updateChannel(Channel* channel) {
    int fd = channel->fd();

    // 不处理虚拟fd（负数或定时器专用）
    if (fd < 0) {
        return;
    }

    if (fd >= FD_SETSIZE) {
        std::cerr << "[Select][warn] update: fd=" << fd
                  << " >= FD_SETSIZE, skip" << std::endl;
        return;
    }

    FD_CLR (fd, &read_set_);
    FD_CLR (fd, &write_set_);

    if (channel->isReading()) {
        FD_SET(fd, &read_set_);
    }
    if (channel->isWriting()) {
        FD_SET(fd, &write_set_);
    }
}

int SelectMultiplexer::dispatch(int timeout_ms, std::vector<Channel*>& active_channels) {
    // 如果没有有效的fd，直接等待超时
    if (max_fd_ < 0) {
        struct timeval timeout;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        // 使用select进行纯超时等待
        select(0, nullptr, nullptr, nullptr, &timeout);
        return 0;
    }

    // 为 select 准备副本
    fd_set read_fds = read_set_;
    fd_set write_fds = write_set_;
    fd_set error_fds = error_set_;

    // 构造超时；若 timeout_ms < 0 表示无限期等待
    struct timeval timeout;
    struct timeval* ptmo = nullptr;
    if (timeout_ms >= 0) {
        timeout.tv_sec = timeout_ms / 1000;            // 秒
        timeout.tv_usec = (timeout_ms % 1000) * 1000;  // 微秒（非负）
        ptmo = &timeout;
    }

    // 计算 nfds（max_fd_ 可能因为关闭而过期，这里使用当前记录的值）
    int nfds = max_fd_ + 1;

    // 包一层重试逻辑：若因 EBADF 失败，清理后重试一次，避免无限刷屏
    for (int attempt = 0; attempt < 2; ++attempt) {
    int num_events = select(nfds, &read_fds, &write_fds, &error_fds, ptmo);

        if (num_events < 0) {
            if (errno == EINTR) {
                // 被信号中断，重试
                continue;
            }
            if (errno == EBADF) {
                // 清理无效 fd 并重算 max_fd_
                bool cleared = false;
                for (const auto &kv : channels_) {
                    int fd = kv.first;
                    if (fd < 0 || fd >= FD_SETSIZE) continue;
                    if (FD_ISSET(fd, &read_set_) || FD_ISSET(fd, &write_set_) || FD_ISSET(fd, &error_set_)) {
                        if (fcntl(fd, F_GETFD) == -1 && errno == EBADF) {
                            FD_CLR(fd, &read_set_);
                            FD_CLR(fd, &write_set_);
                            FD_CLR(fd, &error_set_);
                            cleared = true;
                            std::cerr << "[Select][fix] cleared invalid fd=" << fd << " from select sets" << std::endl;
                        }
                    }
                }
                if (cleared) {
                    // 重新计算 max_fd_
                    int new_max = -1;
                    int upper = std::min(max_fd_, FD_SETSIZE - 1);
                    for (int fd = upper; fd >= 0; --fd) {
                        if (FD_ISSET(fd, &read_set_) || FD_ISSET(fd, &write_set_) || FD_ISSET(fd, &error_set_)) {
                            new_max = fd;
                            break;
                        }
                    }
                    max_fd_ = new_max;

                    if (max_fd_ < 0) {
                        // 已无有效 fd，按纯超时等待一次（或无限期）
                        if (ptmo) select(0, nullptr, nullptr, nullptr, ptmo);
                        else select(0, nullptr, nullptr, nullptr, nullptr);
                        return 0;
                    }

                    // 重新准备副本与 nfds 再试一次
                    read_fds = read_set_;
                    write_fds = write_set_;
                    error_fds = error_set_;
                    nfds = max_fd_ + 1;
                    continue; // retry
                }
            }
            std::cerr << "[Select][fatal] select failed: " << std::strerror(errno) << std::endl;
            return -1;
        } else if (num_events == 0) {
            return 0;
        } else {
            // 有事件，继续处理
            // 扫描就绪 fd（0..max_fd_）
            for (int fd = 0; fd <= max_fd_; ++fd) {
                if (fd < 0 || fd >= FD_SETSIZE) continue;
                bool is_active = false;
                int active_events = 0;

                if (FD_ISSET(fd, &read_fds)) { active_events |= Channel::kReadEvent; is_active = true; }
                if (FD_ISSET(fd, &write_fds)) { active_events |= Channel::kWriteEvent; is_active = true; }
                if (FD_ISSET(fd, &error_fds)) { active_events |= Channel::kErrorEvent; is_active = true; }

                if (is_active) {
                    auto it = channels_.find(fd);
                    if (it != channels_.end()) {
                        Channel* channel = it->second;
                        channel->ReadyEvents(active_events);
                        active_channels.push_back(channel);
                    }
                }
            }
            return num_events;
        }
    }

    // 理论上不会到达此处
    return 0;
}




} // namespace MiniEventWork
