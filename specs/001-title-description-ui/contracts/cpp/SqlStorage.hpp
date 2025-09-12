#pragma once
#include <optional>
#include <string>
#include <vector>

namespace chat {

struct Timestamp { long long epochMs{0}; };
struct Conversation { std::string conversationId; };
struct Message { std::string messageId; std::string conversationId; Timestamp createdAt; };

enum class MsgStatus { Queued, Sent, Delivered, Read, Failed };

class SqlStorage {
public:
  virtual ~SqlStorage() = default;
  virtual bool init(const std::string& dbPath) = 0;
  virtual void beginTx() = 0;
  virtual void commitTx() = 0;
  virtual void rollbackTx() = 0;
  virtual void putMessage(const Message& msg) = 0;
  virtual std::vector<Message> getMessages(const std::string& conversationId, int limit, std::optional<Timestamp> before) = 0;
  virtual void putConversation(const Conversation& c) = 0;
  virtual void updateMessageStatus(const std::string& messageId, MsgStatus s) = 0;
};

} // namespace chat
