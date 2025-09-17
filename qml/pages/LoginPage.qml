// Placeholder for LoginPage.qml as per T044.
// Elements: TextField(username), PasswordField, Button(login)
// Implementation will be provided by the user.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Page 是一个方便的顶层容器，通常用于导航
Page {
    title: "Login" // 页面标题

    // 使用锚布局让中心容器始终居中
    Frame {
        anchors.centerIn: parent
        width: parent.width / 2 // 宽度为父容器的一半

        // 列布局，使其内部元素垂直排列
        ColumnLayout {
            anchors.fill: parent // 填充整个 Frame
            spacing: 15 // 元素之间的间距

            Label {
                Layout.alignment: Qt.AlignHCenter // 居中对齐
                text: "Welcome Back!"
                font.pixelSize: 24
            }

            // 用户名输入框
            TextField {
                id: usernameField
                Layout.fillWidth: true
                placeholderText: "Username"
                // 绑定：将输入框的文本双向绑定到 ViewModel 的 userName 属性
                text: loginViewModel.userName
                onTextChanged: loginViewModel.userName = text // 显式双向绑定
            }

            // 密码输入框
            TextField {
                id: passwordField
                Layout.fillWidth: true
                placeholderText: "Password"
                echoMode: TextInput.Password // 以密码模式显示
                // 绑定：双向绑定密码
                text: loginViewModel.password
                onTextChanged: loginViewModel.password = text
            }

            // 登录按钮
            Button {
                id: loginButton
                Layout.fillWidth: true
                text: "Login"
                // 行为绑定：点击时，调用 ViewModel 的 login 方法
                onClicked: loginViewModel.login()
                // 状态绑定：当正在登录时，按钮不可用
                enabled: !loginViewModel.isLoggingIn
            }

            // 状态标签
            Label {
                id: statusLabel
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.WordWrap
                // 状态绑定：文本内容来自 ViewModel
                text: loginViewModel.loginStatus
                color: text.includes("successful") ? "green" : "red"
            }

            // 加载指示器
            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                // 状态绑定：可见性由 ViewModel 控制
                visible: loginViewModel.isLoggingIn
            }
        }
    }
}
