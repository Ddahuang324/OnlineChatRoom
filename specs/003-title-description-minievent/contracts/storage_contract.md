# contracts/storage_contract.md

目标：定义 Storage 层（SQLite）对上层暴露的接口与行为保证

接口示例:

struct MessageRecord { /* 与 data-model 中一致 */ };
struct UserRecord { /* 与 data-model 中一致 */ };

class Storage {
public:
  virtual ~Storage() = default;
  virtual bool init(const std::string& dbPath) = 0;
  virtual bool saveMessage(const MessageRecord& msg) = 0;
  virtual std::vector<MessageRecord> loadRecentMessages(const std::string& roomId, int limit) = 0;
  virtual bool saveUser(const UserRecord& u) = 0;
  virtual std::optional<UserRecord> loadUser(const std::string& id) = 0;
};

行为契约:
- init 在首次调用时创建必要表
- saveMessage 在事务中完成，失败时返回 false 并留有可重试/日志记录
- loadRecentMessages 按 localTimestamp 降序返回最近消息
