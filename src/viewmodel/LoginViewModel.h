#pragma once

// Placeholder for the LoginViewModel header as per T029.
// Implementation will be provided by the user.
#include <QObject>
#include <QString>

class LoginViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString userName READ getUserName WRITE setUserName NOTIFY userNameChanged)
    Q_PROPERTY(QString password READ getPassword WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool isLoggingIn READ getIsLoggingIn NOTIFY isLoggingInChanged)
    Q_PROPERTY(QString loginStatus READ getLoginStatus NOTIFY loginStatusChanged)

public:
    explicit LoginViewModel(QObject* parent = nullptr);
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

private:
    QString userName_;
    QString password_;
    bool isLoggingIn_ = false;
    QString loginStatus_;

    void setIsLoggingIn(bool isLoggingIn);
    void setLoginStatus(const QString& loginStatus);
};