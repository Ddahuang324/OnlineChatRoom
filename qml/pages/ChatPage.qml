// ChatPage.qml - 基础聊天页面
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: chatPage
    title: qsTr("Chat")

    // 假设 chatViewModel 从外部传入 (例如通过 Loader 或 Context)
    property var chatViewModel: null

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        // 连接状态显示
        Text {
            id: statusText
            Layout.fillWidth: true
            text: chatViewModel ? (chatViewModel.isConnected ? qsTr("Connected") : qsTr("Disconnected")) : qsTr("No ViewModel")
            color: chatViewModel && chatViewModel.isConnected ? "green" : "red"
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
        }

        // 消息列表
        ListView {
            id: messageListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: ListModel { id: messageModel }
            delegate: Rectangle {
                width: messageListView.width
                height: 40
                color: index % 2 === 0 ? "#f0f0f0" : "#ffffff"
                Text {
                    anchors.centerIn: parent
                    text: model.content
                    font.pixelSize: 14
                }
            }
            clip: true
        }

        // 输入区域
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: messageInput
                Layout.fillWidth: true
                placeholderText: qsTr("Type your message...")
                enabled: chatViewModel && chatViewModel.isConnected
            }

            Button {
                text: qsTr("Send")
                enabled: chatViewModel && chatViewModel.isConnected && messageInput.text.trim() !== ""
                onClicked: {
                    if (chatViewModel) {
                        chatViewModel.sendText(messageInput.text.trim())
                        messageInput.text = ""
                    }
                }
            }

            Button {
                text: qsTr("Send File")
                enabled: chatViewModel && chatViewModel.isConnected
                onClicked: {
                    // TODO: 实现文件选择和发送
                    console.log("Send file not implemented yet")
                }
            }
        }
    }

    // 监听 ViewModel 信号
    Connections {
        target: chatViewModel
        function onMessageReceivedText(text) {
            messageModel.append({ content: qsTr("Received: ") + text })
        }
        function onMessageSendText(text) {
            messageModel.append({ content: qsTr("Sent: ") + text })
        }
        function onConnectionStatusChanged(connected) {
            statusText.text = connected ? qsTr("Connected") : qsTr("Disconnected")
            statusText.color = connected ? "green" : "red"
        }
    }

    // 页面加载时滚动到底部
    onVisibleChanged: {
        if (visible) {
            messageListView.positionViewAtEnd()
        }
    }
}
