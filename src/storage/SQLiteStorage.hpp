#pragma once

// Placeholder for the SQLiteStorage header file as per T013.
// Implementation will be provided by the user.
#include "../../include/storage/Storage.hpp"
#include <sqlite3.h> // SQLite C API
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <shared_mutex>


class SQLiteStorage : public Storage {
public:
    SQLiteStorage();
    ~SQLiteStorage() override;

    bool init(const std::string& dbPath) override;
    bool saveUser(const UserRecord& user) override;
    std::optional<UserRecord> getUser(const std::string& userId) const override;
    bool saveMessage(const MessageRecord& message) override;
    std::vector<MessageRecord> loadRecentMessages(const std::string& roomId, int limit) const override;

    bool beginTransaction() override;
    bool commitTransaction() override;

    bool saveFileProgress(const std::string& fileId, uint64_t offset) override;
    std::optional<uint64_t> loadFileProgress(const std::string& fileId) override;

    bool recover() override;
private:
    sqlite3* m_db = nullptr; // SQLite 数据库连接指针

    std::shared_mutex rwMutex_; // 用于读写操作的共享互斥锁
    std::thread writeThread_; // 用于执行写操作的线程

    std::atomic<bool> stop_ = false; // 标志位，指示写线程是否应停止
    std::queue<std::function<void()>> writeQueue_; // 写操作队列
    std::mutex queueMutex_; // 保护写操作队列的互斥锁

    std::condition_variable queueCv_; // 条件变量，用于唤醒休眠的写线程

    //Private methods
    void startWriter_(); // 启动写线程
    void stopWriter_(); // 停止写线程
    void executeWrite_(std::function<void()>&& task); // 执行写操作
    bool exec_(const std::string& sql); // 执行SQL语句


};