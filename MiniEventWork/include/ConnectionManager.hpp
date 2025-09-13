#pragma once

#include <atomic>
#include <mutex>
#include <unordered_set>

class ConnectionManager {

public:
    // 单例模式获取 ConnectionManager 实例
    static ConnectionManager& getInstance() ;

    // 增加请求计数
    void requestIncrement();
    // 增加响应计数
    void responseIncrement();
    // 增加超时响应计数
    void timeoutResponseIncrement();
    // 增加连接计数
    void connectionIncrement();
    // 减少连接计数
    void connectionDecrement();

    // 获取总请求数
    long long getTotalRequest() const;
    // 获取总响应数
    long long getTotalResponse() const;
    // 获取超时响应数
    long long getTimeoutResponse() const;
    // 获取当前连接数
    unsigned int getConnections() const;
    // Register/unregister actual connection identifiers (fd) for leak detection
    void registerConnection(int fd);
    void unregisterConnection(int fd);
    unsigned int getRegisteredConnectionsCount() const;
    bool hasRegisteredConnection(int fd) const;
    
private:
    // 私有构造函数，防止外部实例化
    ConnectionManager(void) = default;
    // 私有析构函数
    ~ConnectionManager(void) = default;
    // 删除拷贝构造函数，防止拷贝
    ConnectionManager(const ConnectionManager&) = delete;
    // 删除赋值操作符，防止赋值
    ConnectionManager& operator=(const ConnectionManager&) = delete;

     // 使用原子类型确保线程安全
    std::atomic<long long> _total_request{0};
    std::atomic<long long> _total_response{0};
    std::atomic<long long> _timeout_response{0};
    std::atomic<unsigned int> _connections{0};
    // Registered connection fds for leak detection
    mutable std::mutex _reg_mutex;
    std::unordered_set<int> _registered;

};