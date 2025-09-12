#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chat {

using LocalId = long long;

struct Timestamp { long long epochMs{0}; };
struct MessageDraft {
  std::string conversationId;
  std::string contentType; // text|image|file|audio|video
  std::string contentRef;  // inline text or path
};
struct Message {
  std::string messageId;
  std::string conversationId;
  std::string senderId;
  std::string contentType;
  std::string contentRef;
  Timestamp createdAt;
  std::optional<Timestamp> deliveredAt;
  std::optional<Timestamp> readAt;
};

class Subscription {
public:
  virtual ~Subscription() = default;
  virtual void cancel() = 0;
};

class MessageService {
public:
  virtual ~MessageService() = default;
  virtual LocalId enqueueOutgoing(MessageDraft draft) = 0;
  virtual void markDelivered(const std::string& messageId) = 0;
  virtual void markRead(const std::string& messageId) = 0;
  virtual std::vector<Message> listRecent(const std::string& conversationId, int limit, std::optional<Timestamp> before) = 0;
  virtual Subscription* onIncoming(std::function<void(const Message&)> cb) = 0;
};

} // namespace chat
