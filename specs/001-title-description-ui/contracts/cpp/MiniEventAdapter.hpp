#pragma once
#include <functional>
#include <string>
#include <vector>

namespace chat {

struct Timestamp { long long epochMs{0}; };
struct MessageEnvelope {
  std::string messageId;
  std::string conversationId;
  std::string senderId;
  std::string contentType; // text|image|file|audio|video
  std::string contentRef;  // inline text or URI
  Timestamp createdAt;
};

enum class ErrorCode {
  OK=0,
  CONNECTION_TIMEOUT,
  CONNECTION_REFUSED,
  IO_ERROR,
};

struct SendResult {
  bool ok{false};
  std::string messageId;
  ErrorCode error{ErrorCode::OK};
};

class Subscription {
public:
  virtual ~Subscription() = default;
  virtual void cancel() = 0;
};

class MiniEventAdapter {
public:
  virtual ~MiniEventAdapter() = default;
  virtual bool connect(const std::string& endpoint, int timeoutMs) = 0;
  virtual void disconnect() = 0;
  virtual void subscribeConversation(const std::string& conversationId) = 0;
  virtual SendResult sendMessage(const MessageEnvelope& msg) = 0;
  virtual Subscription* onMessage(std::function<void(const MessageEnvelope&)> cb) = 0;
  virtual void setHeartbeatIntervalMs(int interval) = 0;
};

} // namespace chat
