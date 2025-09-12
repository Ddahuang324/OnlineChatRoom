#pragma once
#include <QObject>
#include <QString>
#include <QVariant>
#include <vector>

class ConversationListViewModel : public QObject {
  Q_OBJECT
public:
  Q_PROPERTY(QVariantList conversations READ conversations NOTIFY conversationsChanged)
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void loadMore(long long beforeEpochMs);
  Q_INVOKABLE void togglePin(const QString& conversationId);
  Q_INVOKABLE void setMuted(const QString& conversationId, bool muted);
  QVariantList conversations() const;
signals:
  void conversationsChanged();
  void conversationUpdated(const QString& conversationId);
  void conversationRemoved(const QString& conversationId);
};

class ChatViewModel : public QObject {
  Q_OBJECT
public:
  Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
  Q_PROPERTY(bool isLoadingHistory READ isLoadingHistory NOTIFY isLoadingHistoryChanged)
  Q_INVOKABLE void sendText(const QString& conversationId, const QString& text);
  Q_INVOKABLE void sendFile(const QString& conversationId, const QString& filePath);
  Q_INVOKABLE void loadHistory(const QString& conversationId, long long beforeEpochMs);
  Q_INVOKABLE void retryMessage(long long localId);
  QVariantList messages() const;
  bool isLoadingHistory() const;
signals:
  void messagesChanged();
  void isLoadingHistoryChanged();
  void messageSent(const QString& messageId);
  void messageFailed(long long localId);
  void messageReceived(const QString& messageId);
};

class PresenceViewModel : public QObject {
  Q_OBJECT
public:
  Q_PROPERTY(QVariantMap presenceMap READ presenceMap NOTIFY presenceMapChanged)
  Q_INVOKABLE void fetchPresence(const QVariantList& userIds);
  Q_INVOKABLE void subscribe(const QString& userId);
  QVariantMap presenceMap() const;
signals:
  void presenceMapChanged();
  void presenceChanged(const QString& userId, const QString& state);
};
