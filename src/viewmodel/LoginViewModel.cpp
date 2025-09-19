// Placeholder for the LoginViewModel implementation as per T030.
// Implementation will be provided by the user.
#include "viewmodel/LoginViewModel.h"
#include <QDebug>



LoginViewModel::LoginViewModel(std::shared_ptr<IMiniEventAdapter> adapter, std::shared_ptr<Storage> storage, QObject* parent)
    : QObject(parent), adapter_(adapter), storage_(storage)
{
    connect(adapter_.get(), &IMiniEventAdapter::connectionEvent, this, &LoginViewModel::handleConnectionEvent);
    connect(adapter_.get(), &IMiniEventAdapter::loginResponse, this, &LoginViewModel::handleLoginResponse);
    
    // 加载保存的凭据
    if (storage_) {
        auto credentials = storage_->loadCredentials();
        if (credentials) {
            userName_ = QString::fromStdString(credentials->first);
            password_ = QString::fromStdString(credentials->second);
            emit userNameChanged();
            emit passwordChanged();
        }
    }
}

QString LoginViewModel::getUserName() const {
    return userName_;
}
QString LoginViewModel::getPassword() const {
    return password_;
}
bool LoginViewModel::getIsLoggingIn() const {
    return isLoggingIn_;
}

QString LoginViewModel::getLoginStatus() const {
    return loginStatus_;
}


void LoginViewModel::setUserName(const QString& userName) {
    if (userName_ == userName) {
        return;
    }
    userName_ = userName;
    emit userNameChanged();
    
    // 保存凭据到本地存储
    if (storage_ && !password_.isEmpty()) {
        storage_->saveCredentials(userName_.toStdString(), password_.toStdString());
    }
}
void LoginViewModel::setPassword(const QString& password) {
    if (password_ == password) {
        return;
    }
    password_ = password;
    emit passwordChanged();
    
    // 保存凭据到本地存储
    if (storage_ && !userName_.isEmpty()) {
        storage_->saveCredentials(userName_.toStdString(), password_.toStdString());
    }
}

void LoginViewModel::login() {
    qDebug() << "LoginViewModel::login called";
    setIsLoggingIn(true);
    setLoginStatus("Logging in...");
    //TODO: 网络层 MiniEventAdapterImpl 实现登录逻辑
    //想网络层MiniEventAdapter 发起一个登录请求
    //网络层MiniEventAdapterImpl 收到请求后，发起一个登录请求，并将请求放入队列中
    //网络层MiniEventAdapterImpl 启动一个线程，不断从队列中取出请求，并执行请求
    //登录成功后，网络层MiniEventAdapterImpl 调用 setLoginStatus() 通知 UI 层 LoginViewModel 登录成功
    //UI 层 LoginViewModel 调用 getLoginStatus() 获取登录状态，并更新 UI 显示
    ConnectionParams params = {"127.0.0.1", 8080};
    adapter_->connect(params);
}

void LoginViewModel::setIsLoggingIn(bool isLoggingIn) {
    if (isLoggingIn_ == isLoggingIn) {
        return;
    }
    isLoggingIn_ = isLoggingIn;
    emit isLoggingInChanged();
}
void LoginViewModel::setLoginStatus(const QString& loginStatus) {
    if (loginStatus_ == loginStatus) {
        return;
    }
    loginStatus_ = loginStatus;
    emit loginStatusChanged();
}



void LoginViewModel::handleConnectionEvent(connectionEvents event) {
   qDebug() << "handleConnectionEvent:" << static_cast<int>(event);
   if(event == connectionEvents::Connected){//连接成功
       setLoginStatus("Connected to server");
       //连接成功后，发起登录请求
       LoginRequest req;
       req.username = getUserName().toStdString();
       req.password = getPassword().toStdString();  // 注意：密码应加密传输
       adapter_->sendLoginRequest(req);
   }else if(event == connectionEvents::Disconnected){
       setLoginStatus("Disconnected from server");
       setIsLoggingIn(false);
   }
}

void LoginViewModel::handleMessage(const MessageRecord& msg) {
    // 登录相关消息现在通过 handleLoginResponse 处理
    // 这里可以处理其他消息类型，如聊天消息
    if (msg.type == MessageType::TEXT) {
        // 处理文本消息（如果需要）
    }
}

void LoginViewModel::handleLoginResponse(const LoginResponse& resp) {
    // 输出登录响应的结果，包括成功状态和消息
    qDebug() << "handleLoginResponse success:" << resp.success << " msg:" << QString::fromStdString(resp.message);
    // 设置登录状态为响应消息
    setLoginStatus(QString::fromStdString(resp.message));
    // 将正在登录的状态设置为 false
    setIsLoggingIn(false);

    if(resp.success){
        // 登录成功后的处理逻辑
        setLoginStatus("Login successful");
        qDebug() << "Emitting loginSuccessful signal";
        emit loginSuccessful();  // 发射信号，通知跳转到聊天页面
    } else {
        qDebug() << "Login failed, not emitting signal";
    }
   
}
