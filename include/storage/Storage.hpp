#pragma once

// Placeholder for the Storage abstract interface as per T012.
// New functions: beginTransaction()/commitTransaction(), saveFileProgress/loadFileProgress
// Implementation will be provided by the user.
#include "model/Entities.hpp"
#include <vector>
#include <string>
#include <optional>

struct UserRecord;
struct MessageRecord;
struct FileMeta;
struct Room;

class Storage {
public:

// 存储接口类，定义了对用户和消息数据的基本操作
virtual ~Storage() = default;

// 初始化存储系统，参数为数据库路径
virtual bool init(const std::string& dbPath) = 0;

// 保存用户记录，参数为用户记录对象
virtual bool saveUser(const UserRecord& user) = 0;

// 根据用户ID获取用户记录，返回可选的用户记录对象
virtual std::optional<UserRecord> getUser(const std::string& userId) const = 0;

// 保存消息记录，参数为消息记录对象
virtual bool saveMessage(const MessageRecord& message) = 0;

// 加载指定房间的最近消息记录，参数为房间ID和消息数量限制
virtual std::vector<MessageRecord> loadRecentMessages(const std::string& roomId, int limit) const = 0;

// 开始数据库事务
virtual bool beginTransaction() = 0;

// 提交数据库事务
virtual bool commitTransaction() = 0;
// 保存文件上传/下载进度
virtual bool saveFileProgress(const std::string& fileId, uint64_t offset) = 0;

// 加载文件上传/下载进度
virtual std::optional<uint64_t> loadFileProgress(const std::string& fileId) = 0;

// (可选) 恢复机制
virtual bool recover() = 0;



};