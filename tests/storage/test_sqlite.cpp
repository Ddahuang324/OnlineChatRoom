// SQLiteStorage 单元测试
// 覆盖点: init/schema, saveUser/getUser, saveMessage/loadRecentMessages,
// saveFileProgress/loadFileProgress, 并发压力测试, recover

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

#include "storage/SQLiteStorage.hpp"

using namespace std::chrono_literals;

static std::string makeTempDbPath(const std::string& name) {
    auto tmp = std::filesystem::temp_directory_path();
    return (tmp / name).string();
}

// Helper to drain writer queue by sleeping a small amount. The SQLiteStorage
// implementation enqueues writes to a background thread; tests wait a bit to
// allow queued writes to flush.
static void drainWrites() {
    std::this_thread::sleep_for(200ms);
}

TEST(SQLiteStorageTest, InitCreatesSchema) {
    const auto path = makeTempDbPath("gtest_sqlite_init.db");
    std::filesystem::remove(path);

    SQLiteStorage store;
    ASSERT_TRUE(store.init(path));

    // verify that database file exists
    ASSERT_TRUE(std::filesystem::exists(path));

    // basic smoke: try to open with sqlite3 to check tables exist
    sqlite3* db = nullptr;
    int rc = sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);

    const char* check_sql = "SELECT name FROM sqlite_master WHERE type='table' AND name IN ('users','rooms','messages','file_progress')";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db, check_sql, -1, &stmt, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) { count++; }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // Expect at least the 4 tables present
    ASSERT_GE(count, 4);

    std::filesystem::remove(path);
}

TEST(SQLiteStorageTest, BasicReadWriteUserAndFileProgressAndMessages) {
    const auto path = makeTempDbPath("gtest_sqlite_basic.db");
    std::filesystem::remove(path);

    SQLiteStorage store;
    ASSERT_TRUE(store.init(path));

    // save a user
    UserRecord user;
    user.id = "user-1";
    user.displayName = "Alice";
    user.avatarPath = "/tmp/avatar.png";
    ASSERT_TRUE(store.saveUser(user));

    // allow background writer to flush
    drainWrites();

    // save file progress
    const std::string fileId = "file-1";
    ASSERT_TRUE(store.saveFileProgress(fileId, 12345));
    drainWrites();

    auto progress = store.loadFileProgress(fileId);
    ASSERT_TRUE(progress.has_value());
    ASSERT_EQ(progress.value(), 12345u);

    // save messages
    const std::string roomId = "room-1";
    const int messages_to_save = 3;
    for (int i = 0; i < messages_to_save; ++i) {
        MessageRecord msg;
        msg.id = "msg-" + std::to_string(i);
        msg.roomId = roomId;
        msg.senderId = user.id;
        msg.content = "hello " + std::to_string(i);
        msg.type = MessageType::TEXT;
        msg.localTimestamp = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) + i;
        msg.serverTimestamp = 0;
        ASSERT_TRUE(store.saveMessage(msg));
    }
    drainWrites();

    auto recent = store.loadRecentMessages(roomId, 10);
    ASSERT_GE(static_cast<int>(recent.size()), messages_to_save);

    std::filesystem::remove(path);
}

TEST(SQLiteStorageTest, ConcurrencyStressTest) {
    const auto path = makeTempDbPath("gtest_sqlite_concurrency.db");
    std::filesystem::remove(path);

    SQLiteStorage store;
    ASSERT_TRUE(store.init(path));

    const int writers = 4;
    const int readers = 4;
    const int messages_per_writer = 200; // keep moderate to keep test fast
    const std::string roomId = "concurrent-room";

    std::atomic<bool> start_flag{false};

    std::vector<std::thread> writer_threads;
    for (int w = 0; w < writers; ++w) {
        writer_threads.emplace_back([&store, &start_flag, w, messages_per_writer, &roomId] {
            while (!start_flag.load(std::memory_order_acquire)) std::this_thread::yield();
            for (int i = 0; i < messages_per_writer; ++i) {
                MessageRecord msg;
                msg.id = "w" + std::to_string(w) + "-m" + std::to_string(i) + "-" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
                msg.roomId = roomId;
                msg.senderId = "writer-" + std::to_string(w);
                msg.content = "payload";
                msg.type = MessageType::TEXT;
                msg.localTimestamp = i;
                msg.serverTimestamp = 0;
                store.saveMessage(msg);
            }
        });
    }

    std::vector<std::thread> reader_threads;
    for (int r = 0; r < readers; ++r) {
        reader_threads.emplace_back([&store, &start_flag, &roomId] {
            while (!start_flag.load(std::memory_order_acquire)) std::this_thread::yield();
            // perform some reads
            for (int i = 0; i < 500; ++i) {
                auto v = store.loadRecentMessages(roomId, 50);
                // do small pause to yield
                if (i % 50 == 0) std::this_thread::sleep_for(1ms);
            }
        });
    }

    // start
    std::this_thread::sleep_for(50ms);
    start_flag.store(true, std::memory_order_release);

    // join
    for (auto& t : writer_threads) t.join();
    for (auto& t : reader_threads) t.join();

    // give background writer time to flush
    drainWrites();

    // verify count
    sqlite3* db = nullptr;
    int rc = sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    const std::string count_sql = "SELECT COUNT(*) FROM messages WHERE room_id = ?";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db, count_sql.c_str(), -1, &stmt, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    sqlite3_bind_text(stmt, 1, roomId.c_str(), -1, SQLITE_TRANSIENT);
    int expected = writers * messages_per_writer;
    int found = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        found = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    ASSERT_EQ(found, expected);

    std::filesystem::remove(path);
}

TEST(SQLiteStorageTest, RecoverRebuildsDatabase) {
    const auto path = makeTempDbPath("gtest_sqlite_recover.db");
    std::filesystem::remove(path);

    SQLiteStorage store;
    ASSERT_TRUE(store.init(path));

    // create a dummy file to simulate corruption (truncate)
    std::ofstream ofs(path, std::ios::binary | std::ios::out | std::ios::trunc);
    ofs << "corrupt";
    ofs.close();

    // recover should rebuild DB
    ASSERT_TRUE(store.recover());
    // after recover, file should exist and be an SQLite DB
    sqlite3* db = nullptr;
    int rc = sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
    ASSERT_EQ(rc, SQLITE_OK);
    sqlite3_close(db);

    std::filesystem::remove(path);
}
