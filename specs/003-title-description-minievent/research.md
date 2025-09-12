# Research: 技术调研要点

- MiniEvent 能力检查
  - 需要了解连接模型（客户端/服务器）、消息序列化格式（JSON/Binary）、事件回调模型、重连/心跳策略、传输可靠性保证
- SQLite 与多线程
  - SQLite 写操作需序列化，建议使用单写多读队列或 WAL 模式并限制并发写
- 文件传输策略
  - 分片大小（例如 64KB）、并发分片数、校验（哈希）、断点续传
- Qt/QML 与 C++ 交互
  - 使用 QQmlContext 或 Q_PROPERTY + signals/slots 连接 ViewModel 与 QML
- 打包/跨平台
  - Desktop: CMake + Qt6/Qt5 配置；Mobile: 额外考虑 Qt for Android/iOS 构建链
