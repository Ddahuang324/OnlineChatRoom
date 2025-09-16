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
 MiniEventAdapter (基于 MiniEventWork 的实现要点)
  - init/connect/线程模型:
    - `bool init(EventBase* sharedBase = nullptr)`：适配器可以接受一个外部共享的 `MiniEventWork::EventBase`（例如应用已有的 I/O 循环），如果未提供则在内部 new 一个 `EventBase` 并在单独线程运行 `loop()`。
    - 所有 socket/BufferEvent/Channel 的生命周期必须在 `EventBase` 的线程内管理；通过 `EventBase::queueInLoop` 或 `runInLoop` 将创建/销毁/读写任务封送到 loop 线程。
  - 发送/接收策略:
    - 外部调用 `sendMessage` / `sendFileChunk` 时仅把任务排入线程安全的发送队列（`std::deque` + mutex），并使用 `EventBase::queueInLoop` 在 loop 线程调度实际的写操作，写操作使用 `Buffer::append` + BufferEvent 写 fd。
    - 接收在 BufferEvent 的 read 回调中解析帧（可用 `Buffer::find_str` 或自定义定界符），在 loop 线程直接构建 `SerializedMessage` 后调用 msgCb_（或通过 `queueInLoop` 转发到 ViewModel 主线程）。
  - 统计与日志:
    - 在关键事件（connect, disconnect, message sent/received, timeouts）调用 `ConnectionManager::getInstance()` 的统计接口增量；使用 `MiniEventLog` 记录详细调试/错误信息，测试允许注入输出流以捕获日志行。
  - 错误与重连策略:
    - connect 失败或短暂断开使用指数退避（base=500ms, factor=2, max=30s, attempts<=8）；区分可重试错误（临时网络故障）与不可重试（参数/序列化错误）。
    - 定义错误映射表：socket errno -> NetStatus（见 `docs/network-error-mapping.md`）。
  - API 建议（头文件 `include/net/MiniEventAdapter.h`）:
    - enum class NetStatus { Ok, NotConnected, Timeout, QueueFull, SerializeError, InternalError };
    - struct ConnectionParams { std::string host; uint16_t port; unsigned timeoutMs; bool useSharedEventBase; };
    - virtual NetStatus connect(const ConnectionParams& params) = 0;
    - virtual void disconnect() = 0;
    - virtual NetStatus sendMessage(const SerializedMessage& msg) = 0;
    - virtual NetStatus sendFileChunk(const FileChunk& chunk) = 0;
    - virtual void setMessageHandler(std::function<void(const SerializedMessage&)> cb) = 0;
    - virtual void setConnectionHandler(std::function<void(ConnectionEvent)> cb) = 0;

 实施步骤（MiniEventWork 适配）:
 1. 研究/对齐 MiniEventWork API（已完成）: 确认 `EventBase`, `Buffer/BufferEvent`, `MessageHandler`, `ConnectionManager`, `MiniEventLog` 的可用函数与线程约束。
 2. 在 `include/net/` 中添加 `MiniEventAdapter.h`（抽象契约）并在 `src/net/` 中添加 `MiniEventAdapterMiniEventWork.{h,cpp}` 实现。
 3. 实现连接/接收/发送/断开四个核心流程，严格把 I/O 操作封装在 `EventBase` 线程内，外部线程通过 `queueInLoop` 交互。
 4. 编写 `tests/mocks/FakeMiniEventServer` 基于 `MiniEventWork::TCPSever`，并复用 `TCPMsgHandler` 作为回显/ACK 服务器。
 5. 编写单元测试 `tests/net/test_mini_event_adapter.cpp` 覆盖场景（正常/错误/队列满/断线重连/分片）。
 6. 撰写集成文档 `docs/mini_event_adapter_integration.md`，包含注意事项和示例代码。


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
