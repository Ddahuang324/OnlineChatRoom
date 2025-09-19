#pragma once

#include "net/IMiniEventAdapter.h"
#include <gmock/gmock.h>

// Mock class for IMiniEventAdapter
class MockMiniEventAdapter : public IMiniEventAdapter {
public:
    MOCK_METHOD(void, connect, (const ConnectionParams& params), (override));
    MOCK_METHOD(void, sendLoginRequest, (const LoginRequest& req), (override));
    MOCK_METHOD(void, sendMessage, (const MessageRecord& msg), (override));
};
