#pragma once

// =======================================================================
// Windows Platform
// =======================================================================
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// 告知链接器链接 ws2_32.lib 库，这是 MSVC 编译器的特性
#pragma comment(lib, "ws2_32.lib")

namespace MiniEventWork {

// 定义统一的 socket 句柄类型
typedef SOCKET socket_t;

// 定义统一的关闭 socket 的宏
#define close_socket(s) closesocket(s)

// 一个辅助类，用于自动初始化和清理 Windows Sockets API
// 利用了 C++ 的 RAII (Resource Acquisition Is Initialization) 特性
class NetworkInitializer {
public:
    NetworkInitializer() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            // 在真实项目中，这里应该抛出异常或记录严重错误并退出
            exit(EXIT_FAILURE);
        }
    }

    ~NetworkInitializer() {
        WSACleanup();
    }
};

} // namespace MiniEventWork

// =======================================================================
// POSIX Platform (Linux, macOS, etc.)
// =======================================================================
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

namespace MiniEventWork {

// 定义统一的 socket 句柄类型
typedef int socket_t;

// 定义统一的关闭 socket 的宏
#define close_socket(s) close(s)

// 在 POSIX 平台下，这个类什么也不做，以保持代码统一性
class NetworkInitializer {};

} // namespace MiniEventWork

#endif
