// Placeholder for the SQLiteStorage implementation as per T014.
// Implementation will be provided by the user.
#include "storage/SQLiteStorage.hpp"
#include <iostream>
#include "common/Log.hpp"
#include <utility>



SQLiteStorage::SQLiteStorage():
   stop_(false)
{ // Constructor implementation
    startWriter_(); // Start the writer thread
}

SQLiteStorage::~SQLiteStorage() {
    stopWriter_(); // Stop the writer thread
    if(m_db){
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool SQLiteStorage:: init(const std::string& dbPath) {
    int rc =sqlite3_open_v2(dbPath.c_str(), &m_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
    
    if (rc != SQLITE_OK) {
        log_error("Failed to open database: %s", sqlite3_errmsg(m_db));
        if(m_db){
            sqlite3_close(m_db);
            m_db = nullptr;
        }
    }
    
    log_debug("Database opened successfully at %s", dbPath.c_str());

    // 2. Set PRAGMAs for performance and reliability.
    if (!exec_("PRAGMA journal_mode=WAL;")) return false;
    if (!exec_("PRAGMA synchronous=NORMAL;")) return false;
    

     const char* schema[] = {
        "CREATE TABLE IF NOT EXISTS users (id TEXT PRIMARY KEY NOT NULL, name TEXT NOT NULL, avatar_url TEXT);",
        "CREATE TABLE IF NOT EXISTS rooms (id TEXT PRIMARY KEY NOT NULL, name TEXT NOT NULL, type INTEGER NOT NULL);",
        "CREATE TABLE IF NOT EXISTS messages (local_id TEXT PRIMARY KEY NOT NULL, server_id TEXT, room_id TEXT NOT NULL, sender_id TEXT NOT NULL, content TEXT NOT NULL, content_type INTEGER NOT NULL, local_ts INTEGER NOT NULL, server_ts INTEGER);",
        "CREATE TABLE IF NOT EXISTS file_progress (file_id TEXT PRIMARY KEY NOT NULL, offset INTEGER NOT NULL DEFAULT 0);"
    };

    for (const char* stmt : schema) {
        if (!exec_(stmt)) return false;
    }

    log_debug("Database created successfully");

    return true;
};


bool SQLiteStorage::exec_(const std::string& sql){
    char* errmsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        log_error("Failed to execute SQL: %s. Error: %s", sql.c_str(), errmsg);
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}


void SQLiteStorage::executeWrite_(std::function<void()>&& task){
        std::unique_lock<std::mutex> lock(queueMutex_);
        writeQueue_.emplace(std::move(task));
        queueCv_.notify_one();
}//异步管道

void SQLiteStorage::stopWriter_() {
   {
    std::unique_lock<std::mutex> lock(queueMutex_);
    stop_ = true;
   }
    queueCv_.notify_one();
    if (writeThread_.joinable()) {
        writeThread_.join();
    }
}
void SQLiteStorage::startWriter_() {
    // Placeholder for starting the writer thread if needed.
    writeThread_ = std::thread([this] {
        log_debug("Writer thread started");

        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                queueCv_.wait(lock, [this] { return stop_ || !writeQueue_.empty(); });
                if (stop_ && writeQueue_.empty()) {
                    log_debug("Writer thread stopping");
                    break;
                }
                auto task = std::move(writeQueue_.front());
                writeQueue_.pop();
            }
            task(  );
        }
    });
}


