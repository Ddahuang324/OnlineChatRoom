// Placeholder for the SQLiteStorage implementation as per T014.
// Implementation will be provided by the user.
#include "storage/SQLiteStorage.hpp"
#include <iostream>
#include "common/Log.hpp"
#include <utility>
#include <filesystem>


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
    dbPath_ = dbPath; // 保存数据库路径
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
    // Removed return statement to match the expected void return type
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
    // Removed return statement to match the expected void return type
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


class StatementGuard {
public:     
    StatementGuard(sqlite3* db, const std::string& sql) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
            log_error("Failed to prepare statement: %s, SQL: %s", sqlite3_errmsg(db), sql.c_str());
            stmt_ = nullptr;
        }
    }
    ~StatementGuard() {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }   
    }
    sqlite3_stmt* get() const { return stmt_; }
    operator bool() const { return stmt_ != nullptr; }

private:
    sqlite3_stmt* stmt_ = nullptr; // Declare stmt_ as a private member
};



bool SQLiteStorage::saveUser(const UserRecord& user) {
   
    executeWrite_([this, user] {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        std::string sql = "INSERT OR REPLACE INTO users (id, name, avatar_url) VALUES (?,?,?)";
        StatementGuard stmt (m_db, sql);
        if (!stmt) {
            return;
        }
       //Sql语句
        sqlite3_bind_text(stmt.get(), 1, user.id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 2, user.displayName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 3, user.avatarPath.c_str(), -1, SQLITE_TRANSIENT);
        
        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            log_error("Failed to execute statement: %s", sqlite3_errmsg(m_db));
            return;
        }
   }); // 异步写入
     return true;
}

// ...existing code...
bool SQLiteStorage::saveMessage(const MessageRecord& message) {
    executeWrite_([this, message] {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        std::string sql = "INSERT OR REPLACE INTO messages (local_id, room_id, sender_id, content, content_type, local_ts, server_ts) VALUES (?, ?, ?, ?, ?, ?, ?)";
        StatementGuard stmt(m_db, sql);
        if (!stmt) {
            return;
        }
        sqlite3_bind_text(stmt.get(), 1, message.id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 2, message.roomId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 3, message.senderId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 4, message.content.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt.get(), 5, static_cast<int>(message.type));
        sqlite3_bind_int64(stmt.get(), 6, message.localTimestamp);
        sqlite3_bind_int64(stmt.get(), 7, message.serverTimestamp);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            log_error("Failed to save message: %s", sqlite3_errmsg(m_db));
        }
    });
    return true;
}


std::vector<MessageRecord> SQLiteStorage::loadRecentMessages(const std::string& roomId, int limit) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);

    std::vector<MessageRecord> messages;
    std::string sql = "SELECT local_id, room_id, sender_id, content, content_type, local_ts, server_ts FROM messages WHERE room_id = ? ORDER BY local_ts DESC LIMIT ?";
    StatementGuard stmt(m_db, sql);
    if (!stmt) {
        return messages;
    }
    sqlite3_bind_text(stmt.get(), 1, roomId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 2, limit);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        MessageRecord msg;
        msg.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        msg.roomId = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        msg.senderId = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        msg.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
        msg.type = static_cast<MessageType>(sqlite3_column_int(stmt.get(), 4));
        msg.localTimestamp = sqlite3_column_int64(stmt.get(), 5);
        msg.serverTimestamp = sqlite3_column_int64(stmt.get(), 6);
        messages.push_back(std::move(msg));
    }
    return messages;
}


bool SQLiteStorage::saveFileProgress(const std::string& fileId, uint64_t offset) {
    executeWrite_([this, fileId, offset] {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        const char* sql = "INSERT OR REPLACE INTO file_progress (file_id, offset) VALUES (?, ?)";
        StatementGuard stmt(m_db, sql);
        if (!stmt) {
            return;
        }
        sqlite3_bind_text(stmt.get(), 1, fileId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt.get(), 2, offset);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            log_error("Failed to save file progress: %s", sqlite3_errmsg(m_db));
        }
    });
}


std::optional<uint64_t>  SQLiteStorage::loadFileProgress(const std::string& fileId)  {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    const char* sql = "SELECT offset FROM file_progress WHERE file_id = ?";
    StatementGuard stmt(m_db, sql);
    if (!stmt) {
        return false;
    }
    sqlite3_bind_text(stmt.get(), 1, fileId.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
       return static_cast<uint64_t>(sqlite3_column_int64(stmt.get(), 0));
    } else if (rc == SQLITE_DONE) {
        return std::nullopt; // No record found
    }else{
        log_error("Failed to load file progress: %s", sqlite3_errmsg(m_db));
        return std::nullopt;
    }
}

bool SQLiteStorage:: recover() {
    // 采取放弃并重建的策略
    log_warn("Recovering database from scratch");
    
    // 停止写线程
    stopWriter_();
    
    // 关闭当前数据库连接
    if(m_db){
        sqlite3_close(m_db);
        m_db = nullptr;
    }
    
    // 备份现有数据库文件
    std::string backupPath = dbPath_ + ".bak";
    if (std::filesystem::exists(dbPath_)) {
        std::filesystem::copy_file(dbPath_, backupPath, std::filesystem::copy_options::overwrite_existing);
        log_info("Database backed up to %s", backupPath.c_str());
    }
    
    // 删除损坏的数据库文件
    if (std::filesystem::exists(dbPath_)) {
        std::filesystem::remove(dbPath_);
    }
    
    // 重新初始化数据库
    if (!init(dbPath_)) {
        log_error("Failed to reinitialize database during recovery");
        return false;
    }
    
    log_info("Database recovery completed");
    return true;
}