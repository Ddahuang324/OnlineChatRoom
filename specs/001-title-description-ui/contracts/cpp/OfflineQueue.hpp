#pragma once
#include <string>
#include <vector>

namespace chat {

using LocalId = long long;

struct MessageDraft {
  std::string conversationId;
  std::string contentType;
  std::string contentRef;
};

enum class ErrorCode { OK=0, RETRY_EXHAUSTED };

class OfflineQueue {
public:
  virtual ~OfflineQueue() = default;
  virtual LocalId push(MessageDraft draft) = 0;
  virtual std::vector<MessageDraft> popReady(size_t maxN) = 0;
  virtual void markFailed(LocalId id, int attempt, ErrorCode ec) = 0;
};

} // namespace chat
