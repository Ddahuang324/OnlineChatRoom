#pragma once

// Placeholder for basic entity structs and enums as per T010.
// Content: UserStatus, MessageType, MessageState enums; struct UserRecord/MessageRecord/FileMeta
// Implementation will be provided by the user.
#include <string>
#include <vector>
#include <memory>

enum class UserStatus {
    OFFLINE,
    ONLINE,
    AWAY,
    BUSY,
    TYPING,
};


enum class MessageType {
    TEXT,
    IMAGE,
    FILE,
};


enum class MessageState {
    Pending,  // 待发送
    Sent,     // 已发送
    Delivered,// 已送达
    Failed    // 发送失败
};

// 前向声明：FileMeta 的完整定义可能在其它头文件中。
struct FileMeta;


// 用户记录
struct UserRecord {
    std::string id;           // 唯一标识
    std::string displayName;  // 显示名称
    std::string avatarPath;   // 头像路径 (本地)
    UserStatus status;        // 用户状态
};

// 消息记录
struct MessageRecord {
    std::string id;               // 消息唯一ID
    std::string roomId;           // 所属房间ID
    std::string senderId;         // 发送者ID
    int64_t localTimestamp;       // 本地时间戳 (毫秒)
    int64_t serverTimestamp;      // 服务器时间戳 (可选, 毫秒)
    MessageType type;             // 消息类型
    std::string content;          // 文本内容 (如果是文本消息)
    std::shared_ptr<FileMeta> fileMeta; // 文件元数据 (如果是文件消息)，使用指针以配合前向声明
    MessageState state;           // 消息状态
};

struct FileMeta {
    std::string fileId;      // 唯一ID（可由服务器或本地生成）
    std::string fileName;    // 文件名
    std::string mimeType;    // MIME 类型
    std::string localPath;   // 本地路径（可选）
    uint64_t    fileSize{0}; // 字节大小
    uint32_t    chunkIndex{0};   // 当前分片序号（如果是分片传输）
    uint32_t    totalChunks{0};  // 总分片数
};