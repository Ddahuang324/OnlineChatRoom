````markdown
# 实现计划: 跨平台现代聊天室

**分支**: `001-title-description-ui` | **日期**: 2025-09-12 | **规格**: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/spec.md
**输入**: 来自 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/spec.md` 的功能规格

## 执行流程 (/plan 命令范围)
1. 从输入路径加载功能规格（绝对路径已在上方）。
2. 填写技术背景并在章程检查中标注任何偏差。
3. 执行阶段 0（research.md）以解决 spec 中的 [需要澄清] 项。
4. 执行阶段 1，生成 data-model.md、contracts/（openapi 协议草案）、quickstart.md。
5. 更新进度追踪并准备阶段 2（由 /tasks 生成 tasks.md）。

---

## 摘要
实现栈: C++ (Qt + QML) 前端，使用内部网络库 `MiniEvent` 作为网络层；支持多媒体（图片/音频/视频）、文件传输、静态存储加密；日志访问不设限制；并发目标用户 ≤ 500。

目标输出（阶段 0-1）:
- /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/research.md
- /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/data-model.md
- /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/contracts/openapi.yaml
- /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/quickstart.md

---

## 技术背景
**语言/版本**: C++ (建议 17 或 20，需要在 research 阶段确认)  
**UI 框架**: Qt (Qt Quick / QML) 用于跨平台桌面与移动界面  
**网络库**: MiniEvent（仓库内已有实现，作为应用的网络层）  
**媒体/文件**: 支持图片、语音、短视频和任意文件传输（需定义最大文件大小）  
**存储**: 本地消息缓存使用 SQLite（或 Qt 的本地存储），并使用静态存储加密（用户选择的密钥或设备密钥）  
**测试**: 单元测试使用现有 CMake + googletest；集成测试需要一个本地 MiniEvent 测试服务器  
**目标平台**: macOS、Windows、Linux（桌面），以及可能的移动（Qt for Android/iOS）——移动支持为可选，需在 research 阶段确认  
**性能目标**: 并发在线用户 ≤ 500；消息延迟目标 <200ms p95（视网络和 MiniEvent 能力）  
**约束**: 日志访问不设限制（已声明），静态存储加密（非 E2EE，除非另行说明）

---

## 章程检查（初始）
- 项目数量: 以单一 repo + 子库模式（UI 库 + core lib + tests）为主，≤3 个主要产物。  
- 直接使用框架: 直接使用 Qt 与 MiniEvent（不再创建过度包装）  
- 测试: 强制 TDD 流程，契约测试优先  
- 可观察性: 结构化日志，统一到本地日志文件或集中服务（实现阶段确认）  

任何与章程的偏差或高复杂度将记录在 `complexity-tracking`（见 research.md）

---

## 项目结构（建议）
```

## UI 架构变更摘要 (MVVM)
已决定采用 MVVM 架构：所有 QML 视图与动画通过 ViewModel（C++）进行绑定和驱动。关键影响：
- 调整 `tasks.md`：新增 ViewModel 接口的契约测试任务与实现任务（见 `specs/001-title-description-ui/contracts/cpp/ViewModels.hpp`）。
- UI 目录结构与部署脚本需更新以编译并注册 C++ ViewModel 到 QML。
- 在实现阶段确保 ViewModel 不进行阻塞 I/O；所有 I/O 经由 core 服务异步完成并通过信号回传。

参见 UI 设计文档: `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/ui-design.md`

src/
├── app/                # Qt/QML 前端 + C++ 后端 glue
├── core/               # 与 MiniEvent 的集成、消息队列、存储、加密
├── media/              # 多媒体处理（缩略图、转码接口适配）
└── tests/

contracts/              # generated API contract 快速草案
specs/001-title-description-ui/  # 本 feature 的规格与计划
```

---

## 阶段 0: 大纲与研究 (research.md)
目标: 解决所有 `spec.md` 中标注的 [需要澄清] 项，并决定关键实现选择。

研究任务示例:
- 确认是否需要 E2EE（用户已选择静态存储加密 -> 默认非 E2EE）
- 确认移动平台支持范围（是否必须支持 Android/iOS）
- 定义文件传输最大大小和分片策略
- 评估 MiniEvent 能否满足并发 500 用户负载与实时推送需求
- 评估静态存储加密的密钥管理方案（设备密钥 vs 用户密码）

输出文件: `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/research.md`

---

## 阶段 1: 设计与契约
先决条件: research.md 已完成

1) data-model.md: 提取用户、会话、消息、群组、文件元数据等实体
   - 输出: `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/data-model.md`

2) contracts: 生成最小 OpenAPI 草案以驱动契约测试
   - 输出: `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/contracts/openapi.yaml`

3) quickstart.md: 本地启动说明（如何构建、运行 MiniEvent 测试服务器、运行前端）
   - 输出: `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/quickstart.md`

阶段 1 输出将使 /tasks 能够生成任务清单

---

## 阶段 2: 任务规划方法（由 /tasks 生成 tasks.md）
- TDD 优先：契约测试 -> 集成测试 -> 实现  
- 任务排序：模型 -> 核心服务 -> MiniEvent 适配层 -> UI/交互  
- 任务估算将基于阶段 1 的结果

---

## 进度追踪
- [ ] 阶段 0: 研究完成
- [ ] 阶段 1: 设计完成
- [ ] 阶段 2: 任务规划完成（/tasks）

````

## 模块与函数级契约（草案）
以下为各子模块的接口契约（函数签名、输入/输出、错误模式、线程/时序约束）。仅为契约，不包含实现细节。

### 核心网络适配: MiniEventAdapter
文件: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/contracts/cpp/MiniEventAdapter.hpp
- connect(const std::string& endpoint, int timeoutMs) -> bool
   - 输入: endpoint(形如 host:port), timeoutMs
   - 输出: true=连接成功；false=失败
   - 错误: CONNECTION_TIMEOUT, CONNECTION_REFUSED
   - 线程: 主线程调用；内部管理 I/O 线程
- disconnect() -> void
- subscribeConversation(const std::string& conversationId) -> void
- sendMessage(const MessageEnvelope& msg) -> SendResult
   - 成功返回 message_id；失败返回错误码
- onMessage(callback<const MessageEnvelope&>) -> Subscription
- setHeartbeatIntervalMs(int interval) -> void

### 消息服务: MessageService
文件: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/contracts/cpp/MessageService.hpp
- enqueueOutgoing(MessageDraft draft) -> LocalId
- markDelivered(const std::string& messageId) -> void
- markRead(const std::string& messageId) -> void
- listRecent(const std::string& conversationId, int limit, Optional<Timestamp> before) -> std::vector<Message>
- onIncoming(callback<const Message&>) -> Subscription

### 本地存储: SqlStorage
文件: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/contracts/cpp/SqlStorage.hpp
- init(const std::string& dbPath) -> bool
- putMessage(const Message& msg) -> void
- getMessages(const std::string& conversationId, int limit, Optional<Timestamp> before) -> std::vector<Message>
- putConversation(const Conversation& c) -> void
- updateMessageStatus(const std::string& messageId, MsgStatus s) -> void
- beginTx() / commitTx() / rollbackTx()

### 静态存储加密: StorageCrypto
文件: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/contracts/cpp/StorageCrypto.hpp
- setKey(const KeyMaterial& key) -> void
- encryptToFile(const std::string& plainPath, const std::string& encPath) -> void
- decryptToFile(const std::string& encPath, const std::string& plainPath) -> void
- encryptBytes(const Bytes& in) -> Bytes
- decryptBytes(const Bytes& in) -> Bytes

### 文件传输: FileUploader
文件: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/contracts/cpp/FileUploader.hpp
- configureChunkSize(size_t bytes) -> void  // 默认 1MB
- upload(const FileMeta& meta, Readable& reader, ProgressCb) -> UploadResult
- resume(const std::string& fileId, Readable& reader, ProgressCb) -> UploadResult

### 离线队列: OfflineQueue
文件: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/contracts/cpp/OfflineQueue.hpp
- push(MessageDraft draft) -> LocalId
- popReady(size_t maxN) -> std::vector<MessageDraft>
- markFailed(LocalId id, int attempt, ErrorCode ec) -> void  // N 默认 5（可配置）

### QML 交互 (前后端 Glue): AppBackend
文件: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/contracts/cpp/AppBackend.hpp
- Q_PROPERTY(QString currentConversation READ currentConversation WRITE setCurrentConversation NOTIFY currentConversationChanged)
- Q_INVOKABLE void sendText(QString text)
- Q_INVOKABLE void sendFile(QString filePath)
- Q_SIGNAL void messageReceived(QString conversationId, QVariant msg)
- Q_SIGNAL void presenceChanged(QString userId, QString state)

---

## 线程模型与时序
- UI 线程: QML/Qt 主线程，仅进行 UI 更新与轻量调用
- I/O 线程: MiniEvent 网络 I/O，保证 onMessage 回调切回到 UI 线程（通过 Qt 的 queued connection）
- 存储线程: 可选后台线程处理 SQLite I/O，接口为异步封装或使用任务队列

## 错误码约定（部分）
```
enum class ErrorCode {
   OK=0,
   CONNECTION_TIMEOUT,
   CONNECTION_REFUSED,
   IO_ERROR,
   STORAGE_CONFLICT,
   ENCRYPTION_FAILED,
   CHUNK_MISMATCH,
   RETRY_EXHAUSTED
};
```

## 性能计数器与日志键
- metrics.minievent.rtt_ms (gauge)
- metrics.msg.send_p95_ms (histogram)
- metrics.sync.catchup_msgs (counter)
- log keys: conv_id, msg_id, user_id, file_id, chunk_idx

## 命名与文件布局（强约束）
- 头文件: `src/<area>/**/<Name>.hpp`，类名 PascalCase，函数名 camelCase
- 所有 ID 使用 UUID v4（字符串，36 长度）
- 时间戳使用 UTC ISO8601 字符串或 64-bit epoch_ms（统一在 data-model.md 声明）

