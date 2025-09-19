// Main QML file for OnlineChat application
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: win
    visible: true
    width: 400
    height: 600
    title: qsTr("Online Chat (Loader Mode)")

    property string currentPage: "login"   // login | chat

    Loader {
        id: mainLoader
        anchors.fill: parent
        source: currentPage === "login" ? "qrc:/qml/pages/LoginPage.qml" : "qrc:/qml/pages/ChatPage.qml"
        onLoaded: {
            console.log("[Loader] loaded", source);
            if (mainLoader.item && currentPage === "chat") {
                mainLoader.item.chatViewModel = chatViewModel;
            }
        }
        onStatusChanged: console.log("[Loader] status", status)
    }

    // 监听登录成功
    Connections {
        target: loginViewModel
        function onLoginSuccessful() {
            console.log("[QML] loginSuccessful received -> switch to chat")
            currentPage = "chat"
        }
    }

    // 调试按钮：手动跳转
    Button {
        text: "Force→Chat"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        visible: currentPage === "login"
        onClicked: {
            console.log("[QML] force button clicked")
            currentPage = "chat"
        }
    }
}
