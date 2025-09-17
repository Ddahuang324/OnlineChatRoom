// Placeholder for the main application entry point as per T043.
// main.cpp: register LoginViewModel, ChatViewModel, set context
// Implementation will be provided by the user.

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <memory>

#include "storage/SQLiteStorage.hpp"
#include "net/MiniEventAdapterImpl.h"
#include "viewmodel/LoginViewModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    // 设置应用信息
    app.setApplicationName("OnlineChat");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("YourOrganization");
    
    // 初始化存储层
    auto storage = std::make_shared<SQLiteStorage>();
    QString dbPath = QDir::homePath() + "/.onlinechat/chat.db";
    
    // 确保数据库目录存在
    QDir dbDir(QDir::homePath() + "/.onlinechat");
    if (!dbDir.exists()) {
        if (!dbDir.mkpath(".")) {
            qCritical() << "Failed to create database directory:" << dbDir.absolutePath();
            return -1;
        }
    }
    
    if (!storage->init(dbPath.toStdString())) {
        qCritical() << "Failed to initialize storage";
        return -1;
    }
    
    // 创建网络适配器
    auto networkAdapter = std::make_shared<MiniEventAdapterImpl>();
    
    // 初始化网络适配器
    networkAdapter->init(nullptr, nullptr, nullptr, nullptr, nullptr);
    
    // 创建登录视图模型，注入存储和网络依赖
    auto loginViewModel = std::make_shared<LoginViewModel>(networkAdapter, storage);
    
    // 创建 QML 引擎
    QQmlApplicationEngine engine;
    
    // 注册类型到 QML
    qmlRegisterUncreatableType<LoginViewModel>("OnlineChat", 1, 0, "LoginViewModel", "Cannot create LoginViewModel from QML");
    
    // 设置上下文属性
    engine.rootContext()->setContextProperty("loginViewModel", loginViewModel.get());
    
    // 加载主 QML 文件
    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);
    
    return app.exec();
}
