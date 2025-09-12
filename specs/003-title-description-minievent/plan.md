# 实施计划: 现代跨平台轻量级网络聊天室（计划文档）

基于 `spec.md` 的功能规格，下面给出阶段化的实施计划与生成工件清单。目标：最小可用产品（MVP）包含登录系统、SQLite 本地存储、文本与文件传输、以及 MiniEvent 的接入，使用 C++17 + Qt/QML，MVVM 架构，多线程。

阶段输出（在本目录生成）:
- stage-0: `research.md`
- stage-1: `data-model.md`, `contracts/network_contract.md`, `contracts/storage_contract.md`, `quickstart.md`
- stage-2: `tasks.md`

进度跟踪: (由本脚本/文档在每阶段完成时更新)

---

## 阶段 0 — Research
- 目标产物: `research.md`
- 关注点: MiniEvent 能力与 API、Qt/QML 最佳实践、SQLite 集成、跨平台打包注意事项、文件传输策略（分片、速率限制）

---

## 阶段 1 — 技术设计
- 产物: `data-model.md`, `contracts/*`, `quickstart.md`
- 目标: 明确数据模型、定义网络与存储合约、提供快速启动说明（如何编译和运行最小演示）

核心模块（按 MVVM 划分）:
- Model 层
  - UserModel
  - MessageModel
  - RoomModel
- ViewModel 层
  - LoginViewModel
  - ChatViewModel
  - RoomsViewModel
- View 层 (QML)
  - LoginPage.qml
  - ChatPage.qml
  - RoomsPage.qml
- Infrastructure
  - MiniEventAdapter (network)
  - Storage (SQLite wrapper)
  - ThreadPool / Worker

函数级别契约（示例）:
- MiniEventAdapter
  - bool connect(const ConnectionParams& params)
  - void disconnect()
  - bool sendMessage(const SerializedMessage& msg)
  - void sendFileChunk(const FileChunk& chunk)
  - void setMessageHandler(std::function<void(const SerializedMessage&)> cb)
  - void setConnectionHandler(std::function<void(ConnectionEvent)> cb)

- Storage (SQLite)
  - bool init(const std::string& dbPath)
  - bool saveMessage(const MessageRecord& msg)
  - std::vector<MessageRecord> loadRecentMessages(const RoomId& room, int limit)
  - bool saveUser(const UserRecord& user)
  - optional<UserRecord> loadUser(const UserId& id)

线程/同步策略:
- 网络 IO 与 MiniEvent 在后台线程
- Storage 操作使用串行化后台队列（避免并发 sqlite 写锁）
- ViewModel 在主线程接收事件并通过 signals/slots 更新 QML

---

## 阶段 2 — 任务拆分
- 产物: `tasks.md`（细化到函数级别、PR 列表与验收标准）

里程碑（建议）:
1. 初始化项目骨架：CMake, Qt Quick 入口, 基础目录结构
2. Storage 模块：SQLite 封装与本地登录会话持久化
3. MiniEventAdapter：连接、发送/接收消息、重连机制
4. ViewModels + QML：Login 页面 + Chat 页面最小可交互原型
5. 文件传输：分片上传/下载 + UI 进度反馈
6. 主题与动画：QML 主题切换与基础消息入场动画
7. 集成测试：本地端到端（模拟 MiniEvent 或使用本地 loopback）

---

后续：`tasks.md` 中会列出每个任务的函数/接口及验收标准。
