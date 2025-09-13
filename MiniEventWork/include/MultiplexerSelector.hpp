#pragma once

#include "IO_Multiplexer.hpp"
#include <memory>

// 选择当前平台上“最快”的 I/O 多路复用实现。
// 策略：
// - 基于编译平台收集候选（Linux: epoll/select; macOS: kqueue/select; Windows: IOCP/select）。
// - 在启动时做一次轻量微基准：连续多次调用 dispatch(0)（非阻塞），统计耗时。
// - 选取平均耗时最短的实现；若某实现初始化或运行失败，自动跳过。
std::unique_ptr<IOMultiplexer> choose_best_multiplexer();
