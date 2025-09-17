#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QCoreApplication>
#include "viewmodel/LoginViewModel.h"
#include "mocks/MockMiniEventAdapter.h"
#include "mocks/MockStorage.h"
#include "model/Entities.hpp"

// Test fixture for LoginViewModel
class LoginViewModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize QCoreApplication if not already done
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char* argv[] = {(char*)"test"};
            app = new QCoreApplication(argc, argv);
        }

        mockAdapter = std::make_shared<MockMiniEventAdapter>();
        mockStorage = std::make_shared<MockStorage>();
        
        // Set default behavior for loadCredentials
        ON_CALL(*mockStorage, loadCredentials())
            .WillByDefault(::testing::Return(std::nullopt));
        
        viewModel = std::make_unique<LoginViewModel>(mockAdapter, mockStorage, nullptr);
    }

    void TearDown() override {
        viewModel.reset();
        mockAdapter.reset();
        mockStorage.reset();
    }

    std::shared_ptr<MockMiniEventAdapter> mockAdapter;
    std::shared_ptr<MockStorage> mockStorage;
    std::unique_ptr<LoginViewModel> viewModel;
    QCoreApplication* app = nullptr;
};

// Test constructor loads credentials
TEST_F(LoginViewModelTest, ConstructorLoadsCredentials) {
    std::pair<std::string, std::string> creds = {"testuser", "testpass"};
    EXPECT_CALL(*mockStorage, loadCredentials())
        .WillOnce(::testing::Return(creds));

    // Recreate viewModel to test constructor
    auto newViewModel = std::make_unique<LoginViewModel>(mockAdapter, mockStorage, nullptr);

    EXPECT_EQ(newViewModel->getUserName().toStdString(), "testuser");
    EXPECT_EQ(newViewModel->getPassword().toStdString(), "testpass");
}

// Test setUserName saves credentials
TEST_F(LoginViewModelTest, SetUserNameSavesCredentials) {
    viewModel->setPassword("pass"); // Set password first
    EXPECT_CALL(*mockStorage, saveCredentials("newuser", "pass"))
        .Times(1);

    viewModel->setUserName("newuser");
    EXPECT_EQ(viewModel->getUserName().toStdString(), "newuser");
}

// Test setPassword saves credentials
TEST_F(LoginViewModelTest, SetPasswordSavesCredentials) {
    viewModel->setUserName("user"); // Set username first
    EXPECT_CALL(*mockStorage, saveCredentials("user", "newpass"))
        .Times(1);

    viewModel->setPassword("newpass");
    EXPECT_EQ(viewModel->getPassword().toStdString(), "newpass");
}

// Test login calls connect
TEST_F(LoginViewModelTest, LoginCallsConnect) {
    ConnectionParams expectedParams = {"127.0.0.1", 8080};
    EXPECT_CALL(*mockAdapter, connect(expectedParams))
        .Times(1);

    viewModel->setUserName("user");
    viewModel->setPassword("pass");
    viewModel->login();

    EXPECT_TRUE(viewModel->getIsLoggingIn());
    EXPECT_EQ(viewModel->getLoginStatus().toStdString(), "Logging in...");
}

// Test handleConnectionEvent connected sends login request
TEST_F(LoginViewModelTest, HandleConnectionEventConnectedSendsLoginRequest) {
    LoginRequest expectedReq = {"user", "pass"};
    EXPECT_CALL(*mockAdapter, sendLoginRequest(expectedReq))
        .Times(1);

    viewModel->setUserName("user");
    viewModel->setPassword("pass");

    viewModel->handleConnectionEvent(connectionEvents::Connected);

    EXPECT_EQ(viewModel->getLoginStatus().toStdString(), "Connected to server");
}

// Test handleConnectionEvent disconnected sets status
TEST_F(LoginViewModelTest, HandleConnectionEventDisconnectedSetsStatus) {
    viewModel->setIsLoggingIn(true);

    viewModel->handleConnectionEvent(connectionEvents::Disconnected);

    EXPECT_EQ(viewModel->getLoginStatus().toStdString(), "Disconnected from server");
    EXPECT_FALSE(viewModel->getIsLoggingIn());
}

// Test handleLoginResponse success
TEST_F(LoginViewModelTest, HandleLoginResponseSuccess) {
    LoginResponse resp = {true, "Login successful", "user123"};

    viewModel->setIsLoggingIn(true);
    viewModel->handleLoginResponse(resp);

    EXPECT_EQ(viewModel->getLoginStatus().toStdString(), "Login successful");
    EXPECT_FALSE(viewModel->getIsLoggingIn());
}

// Test handleLoginResponse failure
TEST_F(LoginViewModelTest, HandleLoginResponseFailure) {
    LoginResponse resp = {false, "Invalid credentials", ""};

    viewModel->setIsLoggingIn(true);
    viewModel->handleLoginResponse(resp);

    EXPECT_EQ(viewModel->getLoginStatus().toStdString(), "Invalid credentials");
    EXPECT_FALSE(viewModel->getIsLoggingIn());
}
