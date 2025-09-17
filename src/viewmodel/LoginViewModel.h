#pragma once

// Placeholder for the LoginViewModel header as per T029.
// Implementation will be provided by the user.
#include <QObject>
#include <QString>
#include "net/IMiniEventAdapter.h"
#include <memory>
#include "storage/Storage.hpp"

class LoginViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString userName READ getUserName WRITE setUserName NOTIFY userNameChanged)
    Q_PROPERTY(QString password READ getPassword WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool isLoggingIn READ getIsLoggingIn NOTIFY isLoggingInChanged)
    Q_PROPERTY(QString loginStatus READ getLoginStatus NOTIFY loginStatusChanged)

public:
    explicit LoginViewModel(std::shared_ptr<IMiniEventAdapter> adapter, std::shared_ptr<Storage> storage, QObject* parent = nullptr);
//read
    QString getUserName() const;
    QString getPassword() const;
    bool getIsLoggingIn() const;
    QString getLoginStatus() const;
//write
    void setUserName(const QString& userName);
    void setPassword(const QString& password);
    
public slots:
    void login();

signals:
    void userNameChanged();
    void passwordChanged();
    void isLoggingInChanged();
    void loginStatusChanged();

public:
    void handleConnectionEvent(connectionEvents event);
    void handleMessage(const MessageRecord& msg);
    void handleLoginResponse(const LoginResponse& resp);

    void setIsLoggingIn(bool isLoggingIn);
    void setLoginStatus(const QString& loginStatus);

protected:
    QString userName_;
    QString password_;
    bool isLoggingIn_ = false;
    QString loginStatus_;

    std::shared_ptr<IMiniEventAdapter> adapter_;
    std::shared_ptr<Storage> storage_;

    friend class LoginViewModelTest;
};