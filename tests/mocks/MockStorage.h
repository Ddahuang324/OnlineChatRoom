#pragma once

#include "storage/Storage.hpp"
#include <gmock/gmock.h>

using Credentials = std::optional<std::pair<std::string, std::string>>;

// Mock class for Storage
class MockStorage : public Storage {
public:
    MOCK_METHOD(bool, init, (const std::string& dbPath), (override));
    MOCK_METHOD(bool, saveUser, (const UserRecord& user), (override));
    MOCK_METHOD(std::optional<UserRecord>, getUser, (const std::string& userId), (const, override));
    MOCK_METHOD(bool, saveMessage, (const MessageRecord& message), (override));
    MOCK_METHOD(std::vector<MessageRecord>, loadRecentMessages, (const std::string& roomId, int limit), (const, override));
    MOCK_METHOD(bool, beginTransaction, (), (override));
    MOCK_METHOD(bool, commitTransaction, (), (override));
    MOCK_METHOD(bool, saveFileProgress, (const std::string& fileId, uint64_t offset), (override));
    MOCK_METHOD(std::optional<uint64_t>, loadFileProgress, (const std::string& fileId), (override));
    MOCK_METHOD(bool, recover, (), (override));
    MOCK_METHOD(bool, saveCredentials, (const std::string& username, const std::string& encryptedPassword), (override));
    MOCK_METHOD(Credentials, loadCredentials, (), (const, override));
};
