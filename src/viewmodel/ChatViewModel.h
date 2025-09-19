#pragma once

// Placeholder for the ChatViewModel header as per T031.
// Implementation will be provided by the user.
#include <QObject>
#include <QVariantMap>
#include <QUuid>
#include "model/Entities.hpp"
#include "net/IMiniEventAdapter.h"

class Storage;
class IMiniEventAdapter;
class RoomViewModel;

struct UserRecord;
struct MessageRecord;

class ChatViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY loggedInChanged)  // 如果需要

public:
    explicit ChatViewModel(Storage* storage, IMiniEventAdapter* adapter, QObject* parent = nullptr);
    ~ChatViewModel();

    Q_INVOKABLE void sendText(const QString& text);
    Q_INVOKABLE void sendFile(const QString& filePath);

    RoomViewModel* roomsViewModel() const;
    void setRoomsViewModel(RoomViewModel* roomsViewModel);

    // 新增：设置当前房间ID
    void setCurrentRoomId(const QString& roomId);

    void setLoggedIn(bool loggedIn);
    bool isConnected() const;
    bool isLoggedIn() const;


signals:
    void messageReceived(const MessageRecord& msg);
    // 新增：方便 QML 直接接收文本内容
    void messageReceivedText(const QString& text);
    void messageSend(const MessageRecord& msg);
    void messageSendText(const QString& text);
    void messageSendStatusUpdated(const MessageRecord& msg);
    void roomsViewModelChanged();
    void connectionStatusChanged(bool connected);
    void loggedInChanged();

private slots:  
    void onIncomingMessage(const MessageRecord& msg);
    void onSendMessageAck(const MessageRecord& msg,qint64 serverMessageID , int code );  // 私有槽：响应当前聊天室的变化
    void onCurrentRoomChanged(const QString& roomId);
    // TODO: Add more send methods for different types of media.
/* 
    void sendImage(const QString& filePath);
    void sendAudio(const QString& filePath);
    void sendVideo(const QString& filePath);
*/
    void handleConnectionEvent(connectionEvents event);
private:
    Storage* storage_;
    IMiniEventAdapter* adapter_;
    QUuid uuidGen_;
    QString currentRoomId_; // 追踪当前正在聊天的房间ID
    RoomViewModel* m_roomsViewModel = nullptr; // 指向RoomsViewModel的指针
    bool isLoggedIn_ = false; // 登录状态
    bool isConnected_ = false; // 连接状态
};