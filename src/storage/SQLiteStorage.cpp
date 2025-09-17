// Placeholder for the SQLiteStorage implementation as per T014.
// Implementation will be provid                if (stop_ && writeQueue_.empty()) {
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
        "CREATE TABLE IF NOT EXISTS file_progress (file_id TEXT PRIMARY KEY NOT NULL, offset INTEGER NOT NULL DEFAULT 0);",
        "CREATE TABLE IF NOT EXISTS credentials (id INTEGER PRIMARY KEY CHECK (id = 1), username TEXT NOT NULL, encrypted_password TEXT NOT NULL);"
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


void SQLiteStorage::executeWrite_(const std::function<void()>& task){
        std::unique_lock<std::mutex> lock(queueMutex_);
        writeQueue_.emplace(task);
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
                task = std::move(writeQueue_.front());
                writeQueue_.pop();
            }
            if(task) {
                task();
            }
        }
    });
}


class StatementGuard {
public:     
    StatementGuard(sqlite3* db, const std::string& sql) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
            // log_error("Failed to prepare statement: %s, SQL: %s", sqlite3_errmsg(db), sql.c_str());
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
    executeWrite_([this, user]() {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        std::string sql = "INSERT OR REPLACE INTO users (id, name, avatar_url) VALUES (?,?,?)";
        StatementGuard stmt(m_db, sql);
        if (!stmt) {
            return;
        }
        sqlite3_bind_text(stmt.get(), 1, user.id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 2, user.displayName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 3, user.avatarPath.c_str(), -1, SQLITE_TRANSIENT);
        
        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            // log_error("Failed to execute statement: %s", sqlite3_errmsg(m_db));
            return;
        }
    });
    return true;
}

// ...existing code...
bool SQLiteStorage::saveMessage(const MessageRecord& message) {
    executeWrite_([this, message]() {
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
            // log_error("Failed to save message: %s", sqlite3_errmsg(m_db));
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

std::optional<UserRecord> SQLiteStorage::getUser(const std::string& userId) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);

    std::string sql = "SELECT id, name, avatar_url FROM users WHERE id = ?";
    StatementGuard stmt(m_db, sql);
    if (!stmt) {
        return std::nullopt;
    }
    sqlite3_bind_text(stmt.get(), 1, userId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        UserRecord user;
        user.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        user.displayName = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        user.avatarPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        return user;
    }
    return std::nullopt;
}

bool SQLiteStorage::beginTransaction() {
    return exec_("BEGIN TRANSACTION;");
}

bool SQLiteStorage::commitTransaction() {
    return exec_("COMMIT;");
}

bool SQLiteStorage::saveFileProgress(const std::string& fileId, uint64_t offset) {
    executeWrite_([this, fileId, offset]() {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        const char* sql = "INSERT OR REPLACE INTO file_progress (file_id, offset) VALUES (?, ?)";
        StatementGuard stmt(m_db, sql);
        if (!stmt) {
            return;
        }
        sqlite3_bind_text(stmt.get(), 1, fileId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt.get(), 2, offset);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            // log_error("Failed to save file progress: %s", sqlite3_errmsg(m_db));
        }
    });
    return true;
}


std::optional<uint64_t>  SQLiteStorage::loadFileProgress(const std::string& fileId)  {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    const char* sql = "SELECT offset FROM file_progress WHERE file_id = ?";
    StatementGuard stmt(m_db, sql);
    if (!stmt) {
        return std::nullopt;
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
    
    // 简化恢复：直接重新初始化数据库（移除文件系统操作以避免编译问题）
    if (!init(dbPath_)) {
        log_error("Failed to reinitialize database during recovery");
        return false;
    }
    
    log_info("Database recovery completed");
    return true;
}

// 简单的加密函数（实际应用中应使用更强的加密算法）
std::string SQLiteStorage::encryptPassword_(const std::string& password) const {
    // 这里使用简单的base64编码作为示例
    // 实际应用中应该使用AES加密或其他强加密算法
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string encoded;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    for (char c : password) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for(i = 0; i < 4; i++)
                encoded += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        
        for (j = 0; j < i + 1; j++)
            encoded += base64_chars[char_array_4[j]];
        
        while((i++ < 3))
            encoded += '=';
    }
    
    return encoded;
}

std::string SQLiteStorage::decryptPassword_(const std::string& encryptedPassword) const {
    // 简单的base64解码（对应上面的加密）
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string decoded;
    int i = 0;
    int j = 0;
    unsigned char char_array_4[4], char_array_3[3];
    
    for (char c : encryptedPassword) {
        if (c == '=') break;
        char_array_4[i++] = base64_chars.find(c);
        if (i == 4) {
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++)
                decoded += char_array_3[i];
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        
        for (j = 0; j < i - 1; j++)
            decoded += char_array_3[j];
    }
    
    return decoded;
}

bool SQLiteStorage::saveCredentials(const std::string& username, const std::string& password) {
    std::string encryptedPassword = encryptPassword_(password);
    std::string sql = "INSERT OR REPLACE INTO credentials (id, username, encrypted_password) VALUES (1, '" + 
                     username + "', '" + encryptedPassword + "');";
    return exec_(sql);
}

std::optional<std::pair<std::string, std::string>> SQLiteStorage::loadCredentials() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    
    sqlite3_stmt* stmt = nullptr;
    std::string sql = "SELECT username, encrypted_password FROM credentials WHERE id = 1;";
    
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        log_error("Failed to prepare statement: %s", sqlite3_errmsg(m_db));
        return std::nullopt;
    }
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        std::string username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string encryptedPassword = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string password = decryptPassword_(encryptedPassword);
        
        sqlite3_finalize(stmt);
        return std::make_pair(username, password);
    }
    
    sqlite3_finalize(stmt);
    return std::nullopt;
}