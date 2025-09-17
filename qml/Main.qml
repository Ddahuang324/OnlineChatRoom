// Main QML file for OnlineChat application
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

ApplicationWindow {
    visible: true
    width: 400
    height: 600
    title: qsTr("Online Chat")

    // Import the LoginPage
    Loader {
        id: pageLoader
        anchors.fill: parent
        source: "qrc:/qml/pages/LoginPage.qml"
    }
}
