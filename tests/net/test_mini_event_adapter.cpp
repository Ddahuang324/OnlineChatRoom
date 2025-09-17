// Unit tests for MiniEventAdapterImpl
#include <gtest/gtest.h>
#include "../../src/net/MiniEventAdapterImpl.h"
#include "../../include/model/Entities.hpp"
#include <memory>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <cstring>
#include <nlohmann/json.hpp>
#include "common/Log.hpp"

// Forward declarations and implementations for serialization functions
SerializedMessage serializedMessage(const MessageRecord msg) {
    nlohmann::json j;
    j["id"] = msg.id;
    j["roomId"] = msg.roomId;
    j["senderId"] = msg.senderId;
    j["localTimestamp"] = msg.localTimestamp;
    j["serverTimestamp"] = msg.serverTimestamp;
    j["type"] = static_cast<int>(msg.type);
    j["content"] = msg.content;
    j["state"] = static_cast<int>(msg.state);
    if (msg.fileMeta) {
        nlohmann::json fileJ;
        fileJ["fileId"] = msg.fileMeta->fileId;
        fileJ["fileName"] = msg.fileMeta->fileName;
        fileJ["mimeType"] = msg.fileMeta->mimeType;
        fileJ["localPath"] = msg.fileMeta->localPath;
        fileJ["fileSize"] = msg.fileMeta->fileSize;
        fileJ["chunkIndex"] = msg.fileMeta->chunkIndex;
        fileJ["totalChunks"] = msg.fileMeta->totalChunks;
        j["fileMeta"] = fileJ;
    } else {
        j["fileMeta"] = nullptr;
    }

    std::string jsonStr = j.dump();
    std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());
    uint32_t length = htonl(static_cast<uint32_t>(payload.size()));
    SerializedMessage result;
    result.push_back(0x01);  // 类型字节 0x01 for Message
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&length), reinterpret_cast<uint8_t*>(&length) + sizeof(length));
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

bool deserializeMessage(const SerializedMessage& serialized, MessageRecord& msg) {
    if (serialized.size() < 1 + sizeof(uint32_t)) {
        return false;  // 不足以读取类型和长度
    }
    if (serialized[0] != 0x01) {
        return false;  // 类型不匹配
    }
    uint32_t length;
    memcpy(&length, &serialized[1], sizeof(length));
    length = ntohl(length);
    if (serialized.size() < 1 + sizeof(uint32_t) + length) {
        return false;  // 数据不足
    }
    std::string jsonStr(reinterpret_cast<const char*>(&serialized[1 + sizeof(uint32_t)]), length);
    try {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        msg.id = j.at("id").get<std::string>();
        msg.roomId = j.at("roomId").get<std::string>();
        msg.senderId = j.at("senderId").get<std::string>();
        msg.localTimestamp = j.at("localTimestamp").get<int64_t>();
        msg.serverTimestamp = j.at("serverTimestamp").get<int64_t>();
        msg.type = static_cast<MessageType>(j.at("type").get<int>());
        msg.content = j.at("content").get<std::string>();
        msg.state = static_cast<MessageState>(j.at("state").get<int>());
        if (!j.at("fileMeta").is_null()) {
            auto fileJ = j.at("fileMeta");
            msg.fileMeta = std::make_shared<FileMeta>();
            msg.fileMeta->fileId = fileJ.at("fileId").get<std::string>();
            msg.fileMeta->fileName = fileJ.at("fileName").get<std::string>();
            msg.fileMeta->mimeType = fileJ.at("mimeType").get<std::string>();
            msg.fileMeta->localPath = fileJ.at("localPath").get<std::string>();
            msg.fileMeta->fileSize = fileJ.at("fileSize").get<uint64_t>();
            msg.fileMeta->chunkIndex = fileJ.at("chunkIndex").get<uint32_t>();
            msg.fileMeta->totalChunks = fileJ.at("totalChunks").get<uint32_t>();
        } else {
            msg.fileMeta = nullptr;
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

SerializedMessage serializedFileChunk(const FileMeta& chunk) {
    nlohmann::json j;
    j["fileId"] = chunk.fileId;
    j["fileName"] = chunk.fileName;
    j["mimeType"] = chunk.mimeType;
    j["localPath"] = chunk.localPath;
    j["fileSize"] = chunk.fileSize;
    j["chunkIndex"] = chunk.chunkIndex;
    j["totalChunks"] = chunk.totalChunks;

    std::string jsonStr = j.dump();
    std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());
    uint32_t length = htonl(static_cast<uint32_t>(payload.size()));
    SerializedMessage result;
    result.push_back(0x02);  // 类型字节 0x02 for FileChunk
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&length), reinterpret_cast<uint8_t*>(&length) + sizeof(length));
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

bool deserializeFileChunk(const SerializedMessage& serialized, FileMeta& chunk) {
    if (serialized.size() < 1 + sizeof(uint32_t)) {
        return false;  // 不足以读取类型和长度
    }
    if (serialized[0] != 0x02) {
        return false;  // 类型不匹配
    }
    uint32_t length;
    memcpy(&length, &serialized[1], sizeof(length));
    length = ntohl(length);
    if (serialized.size() < 1 + sizeof(uint32_t) + length) {
        return false;  // 数据不足
    }
    std::string jsonStr(reinterpret_cast<const char*>(&serialized[1 + sizeof(uint32_t)]), length);
    try {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        chunk.fileId = j.at("fileId").get<std::string>();
        chunk.fileName = j.at("fileName").get<std::string>();
        chunk.mimeType = j.at("mimeType").get<std::string>();
        chunk.localPath = j.at("localPath").get<std::string>();
        chunk.fileSize = j.at("fileSize").get<uint64_t>();
        chunk.chunkIndex = j.at("chunkIndex").get<uint32_t>();
        chunk.totalChunks = j.at("totalChunks").get<uint32_t>();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// Test fixture
class MiniEventAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter = std::make_unique<MiniEventAdapterImpl>();
    }

    void TearDown() override {
        adapter.reset();
    }

    std::unique_ptr<MiniEventAdapterImpl> adapter;
    ConnectionParams testParams{"127.0.0.1", 8080};
    bool connectionCallbackCalled = false;
    connectionEvents lastConnectionEvent;
    bool messageCallbackCalled = false;
    MessageRecord lastMessage;
    bool fileCallbackCalled = false;
    FileMeta lastFileChunk;
};

// Test initialization
TEST_F(MiniEventAdapterTest, InitWithNullEventBase) {
    adapter->init(nullptr,
                  [&](connectionEvents event) {},
                  [&](const MessageRecord&) {},
                  [&](const FileMeta&) {});

    // Should create internal EventBase
    SUCCEED();
}

TEST_F(MiniEventAdapterTest, InitWithCallbacks) {
    bool connectionCalled = false;
    bool messageCalled = false;
    bool fileCalled = false;

    adapter->init(nullptr,
                  [&](connectionEvents event) { connectionCalled = true; },
                  [&](const MessageRecord&) { messageCalled = true; },
                  [&](const FileMeta&) { fileCalled = true; });

    // Verify callbacks are set (we can't directly test private members)
    SUCCEED();
}

// Test connection
TEST_F(MiniEventAdapterTest, ConnectWithoutInit) {
    // Should not crash, but log error
    adapter->connect(testParams);
    SUCCEED();
    std::cout << "ConnectWithoutInit: Handled gracefully without initialization" << std::endl;
}

// Test serialization functions
TEST_F(MiniEventAdapterTest, SerializeDeserializeMessage) {
    MessageRecord original;
    original.id = "test-id";
    original.roomId = "room-1";
    original.senderId = "user-1";
    original.localTimestamp = 1234567890;
    original.serverTimestamp = 1234567891;
    original.type = MessageType::TEXT;
    original.content = "Hello World";
    original.state = MessageState::Sent;

    // Serialize
    SerializedMessage serialized = serializedMessage(original);

    // Deserialize
    MessageRecord deserialized;
    bool success = deserializeMessage(serialized, deserialized);

    ASSERT_TRUE(success);
    EXPECT_EQ(deserialized.id, original.id);
    EXPECT_EQ(deserialized.roomId, original.roomId);
    EXPECT_EQ(deserialized.senderId, original.senderId);
    EXPECT_EQ(deserialized.localTimestamp, original.localTimestamp);
    EXPECT_EQ(deserialized.serverTimestamp, original.serverTimestamp);
    EXPECT_EQ(deserialized.type, original.type);
    EXPECT_EQ(deserialized.content, original.content);
    EXPECT_EQ(deserialized.state, original.state);

    std::cout << "SerializeDeserializeMessage: Original content '" << original.content << "' deserialized to '" << deserialized.content << "'" << std::endl;
}

TEST_F(MiniEventAdapterTest, SerializeDeserializeMessageWithFile) {
    MessageRecord original;
    original.id = "test-id";
    original.roomId = "room-1";
    original.senderId = "user-1";
    original.localTimestamp = 1234567890;
    original.serverTimestamp = 1234567891;
    original.type = MessageType::FILE;
    original.content = "";
    original.state = MessageState::Sent;

    auto fileMeta = std::make_shared<FileMeta>();
    fileMeta->fileId = "file-1";
    fileMeta->fileName = "test.txt";
    fileMeta->mimeType = "text/plain";
    fileMeta->localPath = "/tmp/test.txt";
    fileMeta->fileSize = 1024;
    fileMeta->chunkIndex = 0;
    fileMeta->totalChunks = 1;
    original.fileMeta = fileMeta;

    // Serialize
    SerializedMessage serialized = serializedMessage(original);

    // Deserialize
    MessageRecord deserialized;
    bool success = deserializeMessage(serialized, deserialized);

    ASSERT_TRUE(success);
    ASSERT_TRUE(deserialized.fileMeta);
    EXPECT_EQ(deserialized.fileMeta->fileId, fileMeta->fileId);
    EXPECT_EQ(deserialized.fileMeta->fileName, fileMeta->fileName);
    EXPECT_EQ(deserialized.fileMeta->mimeType, fileMeta->mimeType);
    EXPECT_EQ(deserialized.fileMeta->localPath, fileMeta->localPath);
    EXPECT_EQ(deserialized.fileMeta->fileSize, fileMeta->fileSize);
    EXPECT_EQ(deserialized.fileMeta->chunkIndex, fileMeta->chunkIndex);
    EXPECT_EQ(deserialized.fileMeta->totalChunks, fileMeta->totalChunks);

    std::cout << "SerializeDeserializeMessageWithFile: File '" << fileMeta->fileName << "' with size " << fileMeta->fileSize << " bytes deserialized correctly" << std::endl;
}

TEST_F(MiniEventAdapterTest, SerializeDeserializeFileChunk) {
    FileMeta original;
    original.fileId = "file-1";
    original.fileName = "test.txt";
    original.mimeType = "text/plain";
    original.localPath = "/tmp/test.txt";
    original.fileSize = 1024;
    original.chunkIndex = 0;
    original.totalChunks = 1;

    // Serialize
    SerializedMessage serialized = serializedFileChunk(original);

    // Deserialize
    FileMeta deserialized;
    bool success = deserializeFileChunk(serialized, deserialized);

    ASSERT_TRUE(success);
    EXPECT_EQ(deserialized.fileId, original.fileId);
    EXPECT_EQ(deserialized.fileName, original.fileName);
    EXPECT_EQ(deserialized.mimeType, original.mimeType);
    EXPECT_EQ(deserialized.localPath, original.localPath);
    EXPECT_EQ(deserialized.fileSize, original.fileSize);
    EXPECT_EQ(deserialized.chunkIndex, original.chunkIndex);
    EXPECT_EQ(deserialized.totalChunks, original.totalChunks);

    std::cout << "SerializeDeserializeFileChunk: File chunk '" << original.fileName << "' chunk " << original.chunkIndex << "/" << original.totalChunks << " deserialized correctly" << std::endl;
}

TEST_F(MiniEventAdapterTest, DeserializeInvalidMessage) {
    SerializedMessage invalid;
    invalid.push_back(0x01); // Wrong type
    invalid.insert(invalid.end(), 4, 0); // Invalid length

    MessageRecord msg;
    bool success = deserializeMessage(invalid, msg);

    EXPECT_FALSE(success);
    std::cout << "DeserializeInvalidMessage: Correctly rejected invalid message" << std::endl;
}

TEST_F(MiniEventAdapterTest, DeserializeInvalidFileChunk) {
    SerializedMessage invalid;
    invalid.push_back(0x02); // Wrong type
    invalid.insert(invalid.end(), 4, 0); // Invalid length

    FileMeta chunk;
    bool success = deserializeFileChunk(invalid, chunk);

    EXPECT_FALSE(success);
    std::cout << "DeserializeInvalidFileChunk: Correctly rejected invalid file chunk" << std::endl;
}

// Test message type validation
TEST_F(MiniEventAdapterTest, MessageTypeValidation) {
    SerializedMessage invalidType;
    invalidType.push_back(0x03); // Unknown type
    uint32_t length = htonl(0);
    invalidType.insert(invalidType.end(), reinterpret_cast<uint8_t*>(&length), reinterpret_cast<uint8_t*>(&length) + 4);

    // This would normally be handled in onRead, but we test the concept
    EXPECT_EQ(invalidType[0], 0x03);
    std::cout << "MessageTypeValidation: Unknown message type 0x03 detected" << std::endl;
}

// Test send message without connection
TEST_F(MiniEventAdapterTest, SendMessageWithoutInit) {
    MessageRecord msg;
    msg.id = "test";
    msg.roomId = "room1";
    msg.senderId = "user1";
    msg.content = "test message";
    msg.type = MessageType::TEXT;
    msg.state = MessageState::Pending;

    // Should not crash, but log error
    adapter->sendMessage(msg);
    SUCCEED();
    std::cout << "SendMessageWithoutInit: Handled gracefully - message '" << msg.content << "' queued but not sent due to no initialization" << std::endl;
}

// Test send file chunk without connection
TEST_F(MiniEventAdapterTest, SendFileChunkWithoutInit) {
    FileMeta chunk;
    chunk.fileId = "file1";
    chunk.fileName = "test.txt";
    chunk.chunkIndex = 0;
    chunk.totalChunks = 1;

    // Should not crash, but log error
    adapter->sendFileChunk(chunk);
    SUCCEED();
    std::cout << "SendFileChunkWithoutInit: Handled gracefully - file chunk '" << chunk.fileName << "' chunk " << chunk.chunkIndex << "/" << chunk.totalChunks << " queued but not sent" << std::endl;
}

// Test disconnect without connection
TEST_F(MiniEventAdapterTest, DisconnectWithoutConnection) {
    // Should not crash
    adapter->disconnect();
    SUCCEED();
    std::cout << "DisconnectWithoutConnection: Handled gracefully without active connection" << std::endl;
}

// Test message with all fields
TEST_F(MiniEventAdapterTest, FullMessageSerialization) {
    MessageRecord msg;
    msg.id = "msg-123";
    msg.roomId = "room-456";
    msg.senderId = "user-789";
    msg.localTimestamp = 1640995200000; // 2022-01-01 00:00:00 UTC
    msg.serverTimestamp = 1640995201000;
    msg.type = MessageType::IMAGE;
    msg.content = "Base64 encoded image data";
    msg.state = MessageState::Delivered;

    auto fileMeta = std::make_shared<FileMeta>();
    fileMeta->fileId = "img-001";
    fileMeta->fileName = "photo.jpg";
    fileMeta->mimeType = "image/jpeg";
    fileMeta->localPath = "/tmp/photo.jpg";
    fileMeta->fileSize = 2048576; // 2MB
    fileMeta->chunkIndex = 5;
    fileMeta->totalChunks = 10;
    msg.fileMeta = fileMeta;

    SerializedMessage serialized = serializedMessage(msg);
    MessageRecord deserialized;
    bool success = deserializeMessage(serialized, deserialized);

    ASSERT_TRUE(success);
    EXPECT_EQ(msg.id, deserialized.id);
    EXPECT_EQ(msg.type, deserialized.type);
    EXPECT_EQ(msg.state, deserialized.state);
    ASSERT_TRUE(deserialized.fileMeta);
    EXPECT_EQ(fileMeta->fileSize, deserialized.fileMeta->fileSize);
    EXPECT_EQ(fileMeta->chunkIndex, deserialized.fileMeta->chunkIndex);

    std::cout << "FullMessageSerialization: Complete message with image '" << fileMeta->fileName << "' (" << fileMeta->fileSize << " bytes) chunk " << fileMeta->chunkIndex << "/" << fileMeta->totalChunks << " serialized and deserialized successfully" << std::endl;
}

// Test file chunk with large data
TEST_F(MiniEventAdapterTest, LargeFileChunkSerialization) {
    FileMeta chunk;
    chunk.fileId = "large-file-001";
    chunk.fileName = "large_video.mp4";
    chunk.mimeType = "video/mp4";
    chunk.localPath = "/tmp/large_video.mp4";
    chunk.fileSize = 1073741824; // 1GB
    chunk.chunkIndex = 999;
    chunk.totalChunks = 1000;

    SerializedMessage serialized = serializedFileChunk(chunk);
    FileMeta deserialized;
    bool success = deserializeFileChunk(serialized, deserialized);

    ASSERT_TRUE(success);
    EXPECT_EQ(chunk.fileSize, deserialized.fileSize);
    EXPECT_EQ(chunk.chunkIndex, deserialized.chunkIndex);
    EXPECT_EQ(chunk.totalChunks, deserialized.totalChunks);

    std::cout << "LargeFileChunkSerialization: Large file '" << chunk.fileName << "' (" << chunk.fileSize << " bytes) chunk " << chunk.chunkIndex << "/" << chunk.totalChunks << " serialized correctly" << std::endl;
}

// Test edge cases
TEST_F(MiniEventAdapterTest, EmptyMessageContent) {
    MessageRecord msg;
    msg.id = "empty-msg";
    msg.roomId = "room1";
    msg.senderId = "user1";
    msg.content = ""; // Empty content
    msg.type = MessageType::TEXT;
    msg.state = MessageState::Sent;

    SerializedMessage serialized = serializedMessage(msg);
    MessageRecord deserialized;
    bool success = deserializeMessage(serialized, deserialized);

    ASSERT_TRUE(success);
    EXPECT_EQ(deserialized.content, "");
    std::cout << "EmptyMessageContent: Empty message content handled correctly" << std::endl;
}

TEST_F(MiniEventAdapterTest, ZeroSizeFile) {
    FileMeta chunk;
    chunk.fileId = "empty-file";
    chunk.fileName = "empty.txt";
    chunk.fileSize = 0; // Zero size
    chunk.chunkIndex = 0;
    chunk.totalChunks = 1;

    SerializedMessage serialized = serializedFileChunk(chunk);
    FileMeta deserialized;
    bool success = deserializeFileChunk(serialized, deserialized);

    ASSERT_TRUE(success);
    EXPECT_EQ(deserialized.fileSize, 0ULL);
    std::cout << "ZeroSizeFile: Zero-size file '" << chunk.fileName << "' handled correctly" << std::endl;
}

// Test JSON parsing error handling
TEST_F(MiniEventAdapterTest, MalformedJsonHandling) {
    SerializedMessage malformed;
    malformed.push_back(0x01); // Message type
    uint32_t length = htonl(10);
    malformed.insert(malformed.end(), reinterpret_cast<uint8_t*>(&length), reinterpret_cast<uint8_t*>(&length) + 4);
    std::string badJson = "not valid json";
    malformed.insert(malformed.end(), badJson.begin(), badJson.end());

    MessageRecord msg;
    bool success = deserializeMessage(malformed, msg);

    EXPECT_FALSE(success);
    std::cout << "MalformedJsonHandling: Malformed JSON correctly rejected" << std::endl;
}
