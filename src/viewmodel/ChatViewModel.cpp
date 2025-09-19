// Placeholder for the ChatViewModel implementation as per T032.
// Implementation will be provided by the user.
#include "viewmodel/ChatViewModel.h"
#include "storage/Storage.hpp"
#include "net/IMiniEventAdapter.h"
#include "model/Entities.hpp"
#include <QDebug>
#include <QDateTime>

ChatViewModel::ChatViewModel(Storage* storage, IMiniEventAdapter* adapter, QObject* parent)
    : QObject(parent), storage_(storage), adapter_(adapter), uuidGen_(QUuid::createUuid()) {
    // 连接适配器的信号
    connect(adapter_, &IMiniEventAdapter::connectionEvent, this, &ChatViewModel::handleConnectionEvent);
    connect(adapter_, &IMiniEventAdapter::messageReceived, this, &ChatViewModel::onIncomingMessage);
    // 移除 loginResponse 连接，避免与 LoginViewModel 重复处理
    // connect(adapter_, &IMiniEventAdapter::loginResponse, this, &ChatViewModel::handleLoginResponse);

    // 暂时不连接RoomViewModel，因为它还没有实现
    // if (m_roomsViewModel) {
    //     connect(m_roomsViewModel, &RoomViewModel::currentRoomChanged, this, &ChatViewModel::onCurrentRoomChanged);
    // }
}

ChatViewModel::~ChatViewModel() {
    // 清理资源，如果需要
}

void ChatViewModel::sendText(const QString& text) {
    if (!isLoggedIn_) {
        qWarning() << "Not logged in, cannot send message";
        return;
    }

    if (currentRoomId_.isEmpty()) {
        qWarning() << "No current room selected";
        return;
    }

    if (!isConnected_) {
        qWarning() << "Not connected to chat server, cannot send message";
        return;
    }

    MessageRecord msg;
    msg.id = uuidGen_.createUuid().toString().toStdString();
    msg.roomId = currentRoomId_.toStdString();
    msg.senderId = "";  // TODO: 从登录信息获取
    msg.localTimestamp = QDateTime::currentMSecsSinceEpoch();
    msg.type = MessageType::TEXT;
    msg.content = text.toStdString();
    msg.state = MessageState::Pending;

    // 保存到本地存储
    if (storage_->saveMessage(msg)) {
        // 发送信号
        emit messageSend(msg);
        // 发射文本信号，方便 QML 接收
        emit messageSendText(QString::fromStdString(msg.content));
        // 发送到网络
        adapter_->sendMessage(msg);
    } else {
        qWarning() << "Failed to save message";
    }
}

void ChatViewModel::sendFile(const QString& filePath) {
    // TODO: 实现文件发送
    qDebug() << "sendFile not implemented yet:" << filePath;
}

RoomViewModel* ChatViewModel::roomsViewModel() const {
    return m_roomsViewModel;
}

void ChatViewModel::setRoomsViewModel(RoomViewModel* roomsViewModel) {
    if (m_roomsViewModel != roomsViewModel) {
        m_roomsViewModel = roomsViewModel;
        emit roomsViewModelChanged();
        // TODO: 连接信号当RoomViewModel实现后
    }
}

void ChatViewModel::setLoggedIn(bool loggedIn) {
    if (isLoggedIn_ != loggedIn) {
        isLoggedIn_ = loggedIn;
        emit loggedInChanged();
    }
}

bool ChatViewModel::isConnected() const {
    return isConnected_;
}

bool ChatViewModel::isLoggedIn() const {
    return isLoggedIn_;
}

void ChatViewModel::onIncomingMessage(const MessageRecord& msg) {
    // 保存到存储
    if (storage_->saveMessage(msg)) {
        emit messageReceived(msg);
        emit messageReceivedText(QString::fromStdString(msg.content));
    }
}

void ChatViewModel::onSendMessageAck(const MessageRecord& msg, qint64 serverMessageID, int code) {
    // 更新消息状态
    // TODO: 更新存储中的消息状态
    emit messageSendStatusUpdated(msg);
}

void ChatViewModel::onCurrentRoomChanged(const QString& roomId) {
    currentRoomId_ = roomId;
    // TODO: 加载房间消息
}

void ChatViewModel::setCurrentRoomId(const QString& roomId) {
    if (currentRoomId_ != roomId) {
        currentRoomId_ = roomId;
        // TODO: 加载房间消息
    }
}

// 移除 handleLoginResponse，因为职责分离，由 LoginViewModel 处理

void ChatViewModel::handleConnectionEvent(connectionEvents event) {
    switch (event) {
    case connectionEvents::Connected:
        isConnected_ = true;
        qDebug() << "Connected to chat server";
        emit connectionStatusChanged(true);
        break;
    case connectionEvents::Disconnected:
        isConnected_ = false;
        qDebug() << "Disconnected from chat server";
        emit connectionStatusChanged(false);
        break;
    case connectionEvents::ConnectFailed:
        isConnected_ = false;
        qWarning() << "Failed to connect to chat server";
        emit connectionStatusChanged(false);
        break;
    default:
        break;
    }
}
