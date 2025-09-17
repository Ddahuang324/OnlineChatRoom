#############################################################
# 任务清单 (深度细化: 架构/类/函数/变量/线程/错误模式)
# Feature: MiniEvent 驱动的跨平台轻量聊天 MVP
#############################################################

说明:
- 严格按照 prompt 规则: 设置 → 测试(契约/集成) → 模型 → 服务/适配器 → ViewModel/UI → 传输 → 主题/UI 增强 → 集成 → 完善。
- 不同文件 => 标记 [P] 可并行；同文件/存在依赖不加 [P]。
- 每个任务给出: 涉及文件(精确路径)、核心函数或成员、主要变量/锁/队列、错误模式、验收测试点。
- 枚举/结构体命名与 data-model / contracts 保持一致；新增内部私有成员以下划线结尾说明 (示例: db_, writeQueue_)。

目录基准: repo 根。
FEATURE_DIR: specs/003-title-description-minievent

-------------------------------------------------
阶段 0: 设置 (Build / 目录 / 配置)
-------------------------------------------------
[ ] T001 CMake 顶层初始化 (顺序)
  文件: CMakeLists.txt
  目标: 定义 targets: chat_app, storage_lib, net_lib, viewmodel_lib, transfer_lib, tests
  变量: CMAKE_CXX_STANDARD=17, CHAT_ENABLE_SQLITE(ON), CHAT_ENABLE_QML(ON)
  错误模式: 缺少 Qt/SQLite 找不到包 → FAIL 配置
  验收: 本地执行 cmake .. && cmake --build . 成功

[ ] T002 [P] 构建选项与编译特性分离
  文件: cmake/Options.cmake (新增), CMakeLists.txt (include)
  内容: 选项 CHAT_WARN_AS_ERR, CHAT_BUILD_TESTS, 开启 -Wall -Wextra -Wpedantic
  验收: 关闭/开启选项不报错

[ ] T003 [P] clang-format 与 clang-tidy 配置
  文件: .clang-format, .clang-tidy
  验收: ninja clang-tidy (或脚本) 对 src/ 无严重警告 (除 TODO 抑制)

[ ] T004 [P] 统一日志配置与宏
  文件: src/common/Log.hpp / Log.cpp
  函数: void initLogger(LogLevel level); 宏: LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERR
  变量: static std::atomic<LogLevel> g_logLevel;
  验收: 单元测试捕获日志行格式: [ts][level][thread] message

[ ] T005 配置文件解析模块
  文件: src/config/Config.h / Config.cpp
  函数: bool load(const std::string& path); std::string getString(key); int getInt(key,def)
  变量: unordered_map<std::string,std::string> kv_;
  错误: 文件不存在/键缺失 → 返回默认并日志 WARN
  测试: tests/config/test_config.cpp

-------------------------------------------------
阶段 1: 契约测试 (来自 contracts/*.md) — TDD
-------------------------------------------------
[ ] T006 [P] 网络契约测试 (connect/sendMessage 基线)
  文件: tests/contract/test_network_connect.cpp
  场景: 断开 → connect(host,port) 返回 true; 错误端口 → false
  验收: 使用 FakeMiniEventServer fixture

[ ] T007 [P] 网络契约测试 (文件分片 sendFileChunk)
  文件: tests/contract/test_network_file_chunk.cpp
  断言: 最后分片 last=true 触发回调计数 == 1

[ ] T008 [P] 存储契约测试 (init + saveUser/loadUser)
  文件: tests/contract/test_storage_user.cpp
  验收: 临时 DB; 保存并读取字段一致

[ ] T009 [P] 存储契约测试 (消息写入/读取顺序)
  文件: tests/contract/test_storage_messages.cpp
  断言: loadRecentMessages(limit=2) 时间戳降序

-------------------------------------------------
阶段 2: 数据模型 (来自 data-model.md)
-------------------------------------------------
[ ] T010 [P] 定义基础实体结构体与枚举
  文件: include/model/Entities.hpp
  内容: UserStatus, MessageType, MessageState 枚举; struct UserRecord/MessageRecord/FileMeta
  变量命名: 使用 data-model 字段；新增 helper: static MessageRecord::makeText(...)
  验收: 被下游头引用编译通过

[ ] T011 [P] Room 实体补充 (data-model 中 rooms 表)
  文件: include/model/Room.hpp
  struct Room { std::string id; std::string name; bool isPublic; };
  验收: 编译通过

-------------------------------------------------
阶段 3: Storage 层实现 (接口 → 实现 → 并发)
-------------------------------------------------
[ ] T012 声明 Storage 抽象接口
  文件: include/storage/Storage.h
  新函数: beginTransaction()/commitTransaction(), saveFileProgress/loadFileProgress
  错误: 事务嵌套非法 → 返回 false + 日志 ERROR

[ ] T013 [P] SQLiteStorage 头文件
  文件: src/storage/SQLiteStorage.h
  私有成员: sqlite3* db_=nullptr; std::shared_mutex rwMutex_; std::thread writeThread_; std::atomic<bool> stop_{false}; std::queue<std::function<void()>> writeQueue_; std::mutex queueMutex_; std::condition_variable queueCv_;
  公共: bool init(...); bool saveMessage(...); recover(); 等

[ ] T014 SQLiteStorage::init 实现
  文件: src/storage/SQLiteStorage.cpp
  步骤: open → PRAGMA → create tables (users, rooms, messages, file_progress)
  错误映射: sqlite3_open_v2!=SQLITE_OK → false; 记录 sqlite3_errmsg
  测试依赖: T008

[ ] T015 [P] 写队列线程 & graceful shutdown
  文件: src/storage/SQLiteStorage.cpp
  函数: void startWriter_(); void enqueueWrite_(Fn f);
  验收: 单元测试多线程提交 1000 次 saveMessage 无崩溃

[ ] T016 saveUser/saveMessage/loadRecentMessages 实现
  文件: src/storage/SQLiteStorage.cpp
  SQL: INSERT OR REPLACE; SELECT ... ORDER BY local_ts DESC LIMIT ?
  错误: sqlite3_step != SQLITE_DONE → false

[ ] T017 [P] file_progress 读写实现
  文件: src/storage/SQLiteStorage.cpp
  函数: saveFileProgress/loadFileProgress

[ ] T018 [P] recover() 逻辑
  文件: src/storage/SQLiteStorage.cpp
  步骤: 关闭 db → 复制原文件到 backup → 重新 init

[ ] T019 竞争条件/锁策略审计文档
  文件: docs/storage-concurrency.md
  内容: 队列仅写, 读操作 shared_lock, 写操作 封装为任务串行

[ ] T020 Storage 单元测试实现
  文件: tests/storage/test_sqlite.cpp
  覆盖: init/消息写入/并发/进度/恢复

-------------------------------------------------
阶段 4: 网络适配层 MiniEventAdapter
-------------------------------------------------

[ ] T021 定义 MiniEventAdapter 抽象 (补充状态码与线程模型)
  文件: include/net/MiniEventAdapter.h
  目标: 定义适配器对外契约并记录内部线程模型（EventLoop owned、send queue 串行化）
  函数签名建议:
    - bool init(EventBase* sharedBase = nullptr); // 可传入共享 EventBase 或由适配器 own
    - NetStatus connect(const std::string& host, uint16_t port, unsigned timeoutMs);
    - void disconnect();
    - NetStatus sendMessage(const SerializedMessage& msg); // 序列化并入发送队列
    - NetStatus sendFileChunk(const FileChunk& chunk); // 分片发送，大小 <= 64*1024
    - void setMessageHandler(std::function<void(const SerializedMessage&)> cb);
    - void setConnectionHandler(std::function<void(ConnectionEvent)> cb);
  枚举: NetStatus { Ok, NotConnected, Timeout, QueueFull, SerializeError, InternalError }
  说明: 适配器内部应使用 `MiniEventWork::EventBase` 作为 I/O 循环、`Buffer/BufferEvent` 做 socket IO、`ConnectionManager` 做统计、`MiniEventLog` 做日志。

[ ] T022 Adapter 实现骨架（MiniEventWork 绑定）
  文件: src/net/MiniEventAdapterMiniEventWork.h
  文件: src/net/MiniEventAdapterMiniEventWork.cpp
  私有成员建议:
    - EventBase* eventBase_ = nullptr; // 如果 adapter own，则在 init 中 new
    - std::thread loopThread_; // 当 adapter own EventBase 时用于运行 loop()
    - std::atomic<bool> running_{false};
    - std::mutex sendMutex_; std::condition_variable sendCv_;
    - std::deque<SerializedMessage> sendQueue_; // 使用 deque 便于 peek/pop
    - std::atomic<size_t> maxQueue_{1024};
    - std::function<void(const SerializedMessage&)> msgCb_; std::function<void(ConnectionEvent)> connCb_;
    - ConnectionManager& connStats_ = ConnectionManager::getInstance();
  关键点: 所有真正的 socket read/write 必须在 `EventBase` 线程/回调中执行（使用 runInLoop/queueInLoop），避免跨线程直接调用 Buffer::readFd。

[ ] T023 连接/断开流程实现（使用 EventBase）
  文件: src/net/MiniEventAdapterMiniEventWork.cpp
  行为要求:
    - connect() 发起非阻塞连接：在 loop 线程通过 queueInLoop 建立 BufferEvent/Channel，并注册读写回调。
    - disconnect() 在 loop 线程关闭 BufferEvent、注销 connection 并调用 connCb_(Disconnected)。
    - 指数退避：base=500ms, factor=2, max=30s, attempts<=8，在 connect 失败时使用 eventBase_->executeInWorker/queueInLoop 调度重试。
  错误映射: 系统错误 / socket errno → NetStatus::InternalError 或 Timeout（按 errno 判断），并写入 `docs/network-error-mapping.md`。

[ ] T024 发送队列串行化（在 EventBase 线程执行发送）
  文件: src/net/MiniEventAdapterMiniEventWork.cpp
  逻辑: 外部线程调用 sendMessage() 时先检查 queue 大小（> maxQueue_ 返回 QueueFull），然后通过 eventBase_->queueInLoop 将发送任务入列；真正 send 在 loop 线程调用 Buffer::append/写入 fd（使用 BufferEvent/Channel 回调完成 write）。
  验收: queue 高并发入队（1000 qps）无数据竞争；测试使用单元测试模拟并发调用 sendMessage 并断言没有崩溃。

[ ] T025 接收与消息分发（MessageHandler / TCPMsgHandler）
  文件: src/net/MiniEventAdapterMiniEventWork.cpp
  逻辑: 在 BufferEvent 的 read 回调中使用 `Buffer::find_str` 或自定义帧解析器切分消息，组装为 `SerializedMessage`，再通过 eventBase_->queueInLoop 或直接在 loop 线程调用 msgCb_（若 msgCb_ 能安全在 loop 线程执行，则直接调用以减少拷贝）。
  事件: Connected/Disconnected/Reconnecting，通过 connCb_ 上报，必须携带轻量统计（ConnectionManager::getTotalRequest/Response）。

[ ] T026 sendFileChunk 与分片约束
  文件: src/net/MiniEventAdapterMiniEventWork.cpp
  校验: chunk.bytes.size <= 64*1024; offset 对齐；每个分片发送应写入 storage 进度（回调或 ACK 时回写）。
  异常: 超大分片 → 返回 SerializeError；写入失败 → InternalError 并触发重试策略。

[ ] T027 错误/超时/重连映射文档
  文件: docs/network-error-mapping.md
  内容: 基于 `MiniEventWork` 的错误来源（socket errno、connect timeout、event loop 关闭）与 `NetStatus` 的映射表；记录 reconnect/backoff 策略和可重试错误集合。

[ ] T028 Adapter 单元测试 & Fake Server（基于 MiniEventWork 的 Loopback）
  文件: tests/net/test_mini_event_adapter.cpp
  附件: tests/mocks/FakeMiniEventServer.{cpp,h}（可复用 `MiniEventWork::TCPSever` + `TCPMsgHandler`）
  场景: 成功连接/错误端口/消息往返/队列满/分片上限/断线重连

[ ] T029 文档: MiniEventWork 集成说明
  文件: docs/mini_event_adapter_integration.md
  内容: 列出需要包含的头（EventBase.hpp, Buffer/Buffer.hpp, ConnectionManager.hpp, MiniEventLog.hpp, MessageHandler.hpp），threading contract（哪些函数可跨线程调用，哪些必须在 loop 线程），以及示例代码片段（init、connect、sendMessage、clean shutdown）。


-------------------------------------------------
阶段 5: ViewModel 层 (Login / Chat)
-------------------------------------------------
[ ] T029 LoginViewModel 头文件
  文件: src/viewmodel/LoginViewModel.h
  成员: Q_OBJECT; Storage* storage_; MiniEventAdapter* net_; // 不直接拥有 net
         QThread workerThread_; bool loggingIn_ = false;
  说明: 明确 thread-safety contract：
    - `net_->connect()` 为异步接口（`MiniEventAdapter::connect` 是 `void`），连接结果通过 `ConnectionCallback` 上报。
    - `ConnectionCallback` 可能在 adapter 的 EventBase loop 线程回调，LoginViewModel 必须将回调结果 marshal 到主线程（使用 `QMetaObject::invokeMethod` 或 `QObject::signal` + `Qt::QueuedConnection`）。
  信号: `loginSucceeded()`, `loginFailed(QString)`

[ ] T030 [P] LoginViewModel 实现 login/logout
  文件: src/viewmodel/LoginViewModel.cpp
  目标: 使用非阻塞/异步模式进行连接并安全上报到 QML
  步骤:
    1. 在 UI 触发 login 时，构建 `ConnectionParams` 并调用 `net_->connect(params)`。
       - 如果项目的 `MiniEventAdapter` 实现可能在调用时做较重初始化（阻塞），则通过 `QThread` / `QtConcurrent::run` 在后台线程调用 `connect`；否则可直接调用（适配器约定为非阻塞）。
    2. 在 `init(...)` 时注册 `ConnectionCallback`（在 LoginViewModel 或其代理中），回调收到 `Connected`/`ConnectFailed` 时：
       - 使用 `QMetaObject::invokeMethod(this, "onConnectEvent", Qt::QueuedConnection, ...)` 或发射信号以保证在主线程处理 UI 更新。
       - 在 `Connected` 时发射 `loginSucceeded()`；在 `ConnectFailed` 时发射 `loginFailed(reason)`。
  验收: 在网络成功/失败场景下，QML 层只通过主线程接收 `loginSucceeded/loginFailed`，无跨线程 UI 访问。

[ ] T031 ChatViewModel 头文件
  文件: src/viewmodel/ChatViewModel.h
  成员: Q_OBJECT; Storage* storage_; MiniEventAdapter* net_; // 注入，不直接拥有
         QUuidGenerator uuidGen_; // 或使用 util/Uuid
  说明: ChatViewModel 必须约定对 Adapter 回调的线程策略：所有来自 Adapter 的回调可能在 EventBase 线程调用，ViewModel 需负责 marshal 到主线程并将耗时的存储动作交给 worker thread/Storage 的写队列。
  信号: `messageReceived(QVariantMap)`, `messageSendStatusUpdated(QString messageId, int newState)`

[ ] T032 sendText 实现
  文件: src/viewmodel/ChatViewModel.cpp
  步骤/约定:
    1. 由 UI 在主线程调用 `sendText(const QString& text)`。
    2. 生成消息 UUID -> 构建 `MessageRecord`（state = Pending）-> 同步或通过 Storage 的写队列调用 `storage_->saveMessage(msg)`，确保本地持久化立即可读。
    3. 调用 `net_->sendMessage(msg)`（该接口为异步、返回 void）。
    4. 维护本地状态机：在没有明确 per-message ACK 的情况下，采用乐观更新（Pending -> Sent）并同时设置超时计时器（例如 10s），若超时则将状态置为 Failed 并更新 storage（storage API 必须支持 updateMessageState）。
    5. 若系统设计支持基于回文/ACK 的确认（例如远端回送同 messageId），在接收到对应 ACK 时将状态更新为 Sent 并写回 storage，然后发出 `messageSendStatusUpdated`。
  验收: 单元测试能验证：消息被保存为 Pending、`net_->sendMessage` 被调用、在模拟 ACK 后状态变为 Sent；并在超时后标记为 Failed。

[ ] T033 [P] onIncomingMessage & message 保存
  文件: src/viewmodel/ChatViewModel.cpp
  行为要求:
    - 在 `init` 或构造时注册 `MessageCallback` 到 `MiniEventAdapter`。
    - `MessageCallback` 可能在 adapter 的 EventBase 线程执行：回调中仅做最小处理（解析 messageId/做完整性校验），随后通过 `QMetaObject::invokeMethod` 或 `QCoreApplication::postEvent` 将完整处理委派到主线程或专门的 worker（例如调用 `storage_->saveMessage` 应放到 Storage 的写队列或后台线程）。
    - 完成持久化后，在主线程发射 `messageReceived(QVariantMap)`，QML 直接订阅该信号更新 UI。
  验收: 在接收路径，MessageCallback 在非主线程触发时也不会导致线程问题；消息被持久化且最终在主线程发出 `messageReceived`，携带必要字段 (id, text, sender, ts)

[ ] T034 sendFile 基础实现 (调度 FileTransferManager)
  文件: src/viewmodel/ChatViewModel.cpp
  逻辑: 创建/注入 `FileTransferManager`，并通过其 `uploadFile(path)` 开始上传；ViewModel 仅负责 UI 交互与进度回调转发（进度事件也需要 marshal 到主线程）。
  依赖: T036-T041（FileTransfer 子系统完成）

[ ] T033a RoomsViewModel 头文件
  文件: src/viewmodel/RoomsViewModel.h
  成员: Q_OBJECT; Storage* storage_; // 注入; QVector<Room> rooms_; QString currentRoomId_; 
  说明: 管理房间/聊天对象列表（chat list），提供查询/刷新/选择 room 的接口。必须保证对 Storage 的读取在后台线程或使用 shared_lock 执行。
  信号: `roomsUpdated()`, `roomSelected(QString roomId)`

[ ] T033b [P] RoomsViewModel 实现 (列表/选择)
  文件: src/viewmodel/RoomsViewModel.cpp
  功能: loadRooms(), refreshRooms(), selectRoom(roomId)
  逻辑:
    - loadRooms 从 storage_->loadRooms() 或合适接口读取房间并填充 `rooms_`。
    - selectRoom 更新 `currentRoomId_` 并发射 `roomSelected(roomId)`，ChatViewModel 可监听此信号并切换上下文。
  验收: 在 tests/viewmodel/test_rooms.cpp 中使用 MockStorage 验证加载/选择/刷新行为。

[ ] T035 ViewModel 单元测试
  文件: tests/viewmodel/test_login.cpp / tests/viewmodel/test_chat.cpp
  要求:
    - 使用 `MockStorage` 与 `MockMiniEventAdapter`（MockMiniEventAdapter 必须能够模拟在非主线程触发 `ConnectionCallback` / `MessageCallback`，以验证 ViewModel 的 marshal 行为）。
    - 测试场景：登录成功/失败回调路径、发送文本（保存 Pending + 模拟 ACK -> Sent）、接收消息（跨线程回调 -> 存储 -> 发射 signal）、文件上传触发与进度转发。
  验收: 所有测试在单元环境下通过，且没有直接跨线程的 QObject 操作导致的未定义行为。

-------------------------------------------------
阶段 6: 文件传输子系统
-------------------------------------------------
[ ] T036 FileTransferManager 头文件
  文件: src/transfer/FileTransferManager.h
  成员: struct UploadCtx{ std::string fileId; uint64_t sent; uint64_t total; int retries; uint32_t chunkIndex; uint32_t totalChunks; }; Storage* storage_; MiniEventAdapter* net_; QThreadPool pool_; size_t concurrency_=3;
  说明: 注意 `MiniEventAdapter::sendFileChunk(const FileMeta&)` 的现有签名——`FileMeta` 必须携带足够信息以便适配器在 EventBase 线程中序列化并读取要发送的分片（例如 `localPath`, `fileId`, `chunkIndex`, `totalChunks`, `fileSize`）。

[ ] T037 [P] FileTransferManager.cpp 上传调度 uploadFile
  文件: src/transfer/FileTransferManager.cpp
  分片策略与职责划分:
    - 分片大小上限: 64 KiB
    - `FileTransferManager::uploadFile(path)` 负责:
      1. 生成 `fileId`, 计算 `totalChunks` = ceil(fileSize / 64KiB)
      2. 为每个 chunk 构建 `FileMeta`（至少包含 `fileId`, `fileName`, `localPath`, `fileSize`, `chunkIndex`, `totalChunks`）并提交到线程池或 send 队列。
      3. 不直接假设 adapter 需要 chunk bytes；当前 `MiniEventAdapter` 的签名为 `sendFileChunk(const FileMeta&)`，实现（`MiniEventAdapterImpl`）会在自身的序列化步骤中根据 `localPath`/`chunkIndex` 读取磁盘并构造 `SerializedMessage`。
    - 备选（可选）实现: 若你更倾向于让 `FileTransferManager` 读取并携带 bytes，则应扩展 `FileMeta`（例如添加 `std::vector<uint8_t> chunkData`）并同步更新 `include/model/Entities.hpp` 与 adapter 头以匹配（这会是向后不兼容的 API 变更，需另列任务）。

[ ] T038 onChunkAck 处理与进度保存
  文件: src/transfer/FileTransferManager.cpp
  说明: Adapter 在收到远端 ACK/回执时（由 net 层触发 FileChunkCallback 或 MessageCallback 包含 ACK）应通知 FileTransferManager。FileTransferManager 在确认某分片被接受后调用 `storage_->saveFileProgress(fileId, offsetOrChunkIndex)` 并更新 UploadCtx。测试应覆盖在 ACK 延迟或丢失时的重试逻辑。

[ ] T039 [P] resumeUpload 断点续传
  文件: src/transfer/FileTransferManager.cpp
  逻辑: 调用 `storage_->loadFileProgress(fileId)` 获取最后成功的 chunkIndex/offset，计算下一待发送的 chunkIndex 并继续上传请求。注意：resume 流程必须与 `FileMeta` 中的 `chunkIndex/totalChunks` 保持一致并做边界检查。

[ ] T040 与 ChatViewModel 集成 (注入 manager)
  文件: src/viewmodel/ChatViewModel.cpp / .h
  说明: ChatViewModel 应注入 `FileTransferManager` 实例并在 UI 操作触发时调用 `uploadFile(path)`；同时订阅进度回调并确保这些回调被 marshal 到主线程用于 UI 更新。

[ ] T041 传输重试与指数退避策略
  文件: src/transfer/FileTransferManager.cpp
  算法: attemptDelay = base * 2^retries (base=300ms, max=5s, retries<=3)
  说明: 当 adapter 报告 InternalError/Timeout/Transient 网络错误时进行重试；在达到重试上限后报告失败并更新 message/file 状态为 Failed。

[ ] T042 单元测试文件传输
  文件: tests/transfer/test_file_transfer.cpp
  场景与要求:
    - 正常上传：验证 `FileMeta` 被正确构造并对每个 chunk 调用 `net_->sendFileChunk`；在模拟 ACK 后进度被写入 storage。
    - 中断恢复：在上传中断后，重新启动 upload，验证 `resumeUpload` 从上次保存进度继续。
    - 重试上限：模拟 adapter 返回不可恢复错误，验证在重试用尽后上报失败。
    - 并发上传：启动多个 uploadFile 调用，验证 `QThreadPool`/concurrency 限制生效且对同一 fileId 的分片按序处理（若同一 file 的分片在同一文件/资源上并发写入则应顺序化）。
  Mock 要求: `MockMiniEventAdapter` 必须能够：
    - 模拟在另一个线程（非主线程）触发 ACK 回调/FileChunkCallback，以验证 FileTransferManager 与 ViewModel 的 marshal/同步逻辑；
    - 验证传入的 `FileMeta` 字段完整（包含 `chunkIndex`, `totalChunks`, `localPath` 等）并可选择性地回写 ACK 或模拟错误。

-------------------------------------------------
阶段 7: QML UI / 主题 / 动画
-------------------------------------------------
[ ] T043 主入口 QML + C++ main
  文件: src/app/main.cpp, qml/Main.qml
  main.cpp: 注册 LoginViewModel, ChatViewModel, 设置 context

[ ] T044 [P] LoginPage UI
  文件: qml/pages/LoginPage.qml
  元素: TextField(username), PasswordField, Button(login)

[ ] T045 ChatPage UI 初始
  文件: qml/pages/ChatPage.qml
  元素: ListView(messages), TextArea(input), Button(send), Button(sendFile)

[ ] T045a ChatListPage UI (Chat list / Rooms)
  文件: qml/pages/ChatListPage.qml
  元素: ListView(rooms) 显示房间/联系人缩略信息 (name, lastMessage, unreadCount), Button(openChat)
  交互: 点击房间触发 `roomSelected(roomId)`（绑定到 `RoomsViewModel.roomSelected`），并导航到 `ChatPage.qml`（或在主 QML 中切换 StackView 页面）。
  验收: QML 层能通过 `RoomsViewModel` 的 `roomsUpdated()` 信号刷新列表；点击房间调用 `selectRoom` 并导航。

[ ] T043a main 注册 RoomsViewModel 与导航
  文件: src/app/main.cpp, qml/Main.qml
  说明: main.cpp 应同时注册 `RoomsViewModel`，并在 QML 中实现页面间导航（StackView / Loader）。确保 `RoomsViewModel` 在 app 启动时先做一次 `loadRooms()`。

[ ] T046 [P] 主题定义
  文件: qml/themes/colors.qml, qml/ThemeManager.qml
  变量: primary, background, textPrimary

[ ] T047 消息组件与动画
  文件: qml/components/MessageItem.qml
  动画: 动态淡入 + 位移 12px 内 完成<180ms

-------------------------------------------------
阶段 8: 集成 / 脚本 / 演示
-------------------------------------------------
[ ] T048 Loopback MiniEvent 本地测试脚本
  文件: scripts/loopback_demo.sh
  内容: 启动伪服务器 + 启动应用 2 次

[ ] T049 [P] 集成测试 (端到端：登录+发送文本)
  文件: tests/integration/test_e2e_login_send.cpp
  依赖: Mock/Loopback server

[ ] T050 集成测试 (文件上传 & 断线重连)
  文件: tests/integration/test_e2e_file_reconnect.cpp

-------------------------------------------------
阶段 9: 完善 / 质量 / 文档
-------------------------------------------------
[ ] T051 错误码与日志对照表文档
  文件: docs/error-catalog.md

[ ] T052 [P] 性能基准 (消息写入 & 发送)
  文件: tests/perf/bench_storage_write.cpp, bench_net_send.cpp
  指标: 单条消息写入 < 1ms (本地), send enqueue < 200µs (估算)

[ ] T053 安全检查 (输入验证 + 路径安全)
  文件: docs/security-review.md

[ ] T054 [P] 清理死代码/未使用头
  文件: (全局) 生成报告 docs/code-cleanup.md

[ ] T055 API/模块 README
  文件: README.md (追加模块章节), docs/modules/*.md

[ ] T056 [P] CI 工作流
  文件: .github/workflows/ci.yml
  步骤: build + ctest + artifact (测试报告)

[ ] T057 静态分析/地址消毒器配置
  文件: CMakeLists.txt (添加 -fsanitize=address,undefined 选项开关)

[ ] T058 [P] 发布打包脚本 (最小 bundle)
  文件: scripts/package.sh

[ ] T059 覆盖率报告生成
  文件: scripts/coverage.sh
  工具: gcovr or llvm-cov

[ ] T060 [P] 最终 QA 回归 Checklist
  文件: docs/qa-checklist.md

-------------------------------------------------
阶段 10: Mock / 测试支撑
-------------------------------------------------
[ ] T061 MockStorage (记录调用次数)
  文件: tests/mocks/MockStorage.h

[ ] T062 [P] MockMiniEventAdapter
  文件: tests/mocks/MockMiniEventAdapter.h

[ ] T063 gtest 自定义匹配器
  文件: tests/support/Matchers.h

-------------------------------------------------
阶段 11: 代码生成/工具 (可选优化)
-------------------------------------------------
[ ] T064 统一 UUID 工具
  文件: src/util/Uuid.h / .cpp
  函数: std::string generateV4();

[ ] T065 [P] SHA256 工具 (文件/分片)
  文件: src/util/Sha256.h/.cpp
  验收: 与已知向量匹配

-------------------------------------------------
阶段 12: 回退与恢复策略
-------------------------------------------------
[ ] T066 DB 备份/恢复脚本
  文件: scripts/db_backup.sh
  逻辑: tar sqlite 文件 + 时间戳

[ ] T067 [P] 自动恢复测试
  文件: tests/integration/test_recover_after_corruption.cpp

-------------------------------------------------
阶段 13: 线程 & 内存诊断
-------------------------------------------------
[ ] T068 线程命名辅助
  文件: src/util/ThreadNaming.h/.cpp
  函数: void setCurrentThreadName(const char* name);

[ ] T069 [P] 内存泄漏快速检测文档
  文件: docs/memory-leak-check.md

[ ] T070 TSAN/ASAN 运行脚本
  文件: scripts/run_sanitizers.sh

-------------------------------------------------
阶段 14: 国际化 (最小)
-------------------------------------------------
[ ] T071 国际化占位框架
  文件: src/i18n/I18n.h/.cpp, qml/i18n/strings_zh_CN.qml

-------------------------------------------------
阶段 15: 优化 / 剖析
-------------------------------------------------
[ ] T072 启动路径分析
  文件: docs/startup-profile.md

[ ] T073 [P] CPU 采样脚本适配 (复用已有 scripts/cpu_sample.sh)
  文件: scripts/cpu_sample_chat.sh

-------------------------------------------------
阶段 16: 最终打磨
-------------------------------------------------
[ ] T074 LICENSE / THIRD_PARTY 汇编
  文件: THIRD_PARTY_NOTICES.md

[ ] T075 [P] 自动生成 CHANGELOG
  文件: scripts/gen_changelog.sh

[ ] T076 文档索引
  文件: docs/index.md

[ ] T077 [P] README 快速启动更新
  文件: README.md

[ ] T078 最终端到端冒烟 (脚本)
  文件: scripts/smoke_all.sh

-------------------------------------------------
阶段 17: 退出标准验证
-------------------------------------------------
[ ] T079 覆盖率汇总 >= 70%
  文件: reports/coverage/index.html (生成)

[ ] T080 [P] 生产配置示例
  文件: config/production.example.ini

#############################################################
依赖关系 (摘要):
- T001 → 所有编译相关
- 契约测试 T006-T009 在实现 (T012+ / T021+) 之前
- 模型 (T010-T011) 先于 Storage/Adapter/VM 引用
- Storage: T012 → T013 → (T014,T015,T016,T017,T018) → T020
- Adapter: T021 → T022 → (T023,T024,T025,T026) → T028
- ViewModel: (Storage 完成核心 T016, Adapter 核心 T023/T024) → T029+T030+T031 → T032-T034 → T035
- FileTransfer: 基础依赖 Adapter sendFileChunk (T026) 与 Storage 进度 (T017)
- 集成测试 (T049,T050) 需核心功能 (T032,T037,T038)
- 完善阶段 (T051+) 在核心与集成后

#############################################################
并行执行建议示例:
示例 1 (初始化后):
  可并行: T002 T003 T004 T005
示例 2 (契约阶段):
  可并行: T006 T007 T008 T009
示例 3 (实现中):
  可并行: T013 T021 (不同子系统)
示例 4 (传输扩展):
  可并行: T037 T039 T041 (互不共享文件)
示例 5 (完善):
  可并行: T052 T054 T056 T058 T060

#############################################################
Task Agent 命令示例 (伪示例，若集成自动执行框架，可改成真实脚本):
  agent run T006
  agent run-parallel T006 T007 T008 T009
  agent run T032

#############################################################
验证清单对应:
- 所有契约文件 network_contract.md / storage_contract.md → 测试: T006,T007,T008,T009 ✅
- 所有实体 (User, Message, FileMeta, Room) → T010,T011 ✅
- 测试先于实现: 契约测试在 Storage/Adapter 实现之前编号更小 ✅
- 并行任务仅操作独立文件 (标记 [P]) ✅
- 每任务包含精确文件路径 ✅
- 线程/并发/错误策略有专门文档: T019,T027,T051 ✅

#############################################################
验收退出条件 (T079 触发):
- 所有核心任务 (≤T050) 完成
- 覆盖率 ≥70%
- 集成端到端脚本通过 (T078)
- 无未解决的高严重 bug

#############################################################

#############################################################
# 阶段 18: Agent 生成的摘要任务 (供参考)
#############################################################

## 阶段 1: 设置
- [ ] T081 [P] 根据 `plan.md` 在仓库根目录创建 `src`, `tests`, `gui` 目录结构
- [ ] T082 [P] 在根目录创建基础的 `CMakeLists.txt` 以包含 `src`, `tests`, `gui` 目录, 并链接 Qt 和 MiniEvent
- [ ] T083 [P] 在根目录创建 `.clang-format` 文件以统一代码风格

## 阶段 2: 测试优先 (TDD)
- [ ] T084 [P] 在 `tests/contract/test_storage.cpp` 中根据 `contracts/storage_contract.md` 编写存储层契约测试
- [ ] T085 [P] 在 `tests/contract/test_network.cpp` 中根据 `contracts/network_contract.md` 编写网络层契约测试
- [ ] T086 [P] 在 `tests/integration/test_login.cpp` 中编写登录流程的集成测试 (模拟网络和存储)
- [ ] T087 [P] 在 `tests/integration/test_messaging.cpp` 中编写消息收发的集成测试 (模拟网络和存储)

## 阶段 3: 核心实现
- [ ] T088 [P] 在 `src/model/DataModels.hpp` 中根据 `data-model.md` 创建 `UserRecord`, `MessageRecord`, `FileMeta` 结构体
- [ ] T089 在 `src/storage/` 中创建 `SQLiteStorage.hpp` 和 `SQLiteStorage.cpp`, 实现 `contracts/storage_contract.md` 中定义的 `Storage` 接口
- [ ] T090 在 `src/network/` 中创建 `MiniEventAdapter.hpp` 和 `MiniEventAdapter.cpp`, 实现 `contracts/network_contract.md` 中定义的 `MiniEventAdapter` 接口
- [ ] T091 [P] 在 `src/viewmodel/` 中创建 `LoginViewModel.hpp` 和 `LoginViewModel.cpp`
- [ ] T092 [P] 在 `src/viewmodel/` 中创建 `ChatViewModel.hpp` 和 `ChatViewModel.cpp`
- [ ] T093 [P] 在 `src/viewmodel/` 中创建 `RoomsViewModel.hpp` 和 `RoomsViewModel.cpp`
- [ ] T094 [P] 在 `gui/` 中创建 `LoginPage.qml`
- [ ] T095 [P] 在 `gui/` 中创建 `ChatPage.qml`
- [ ] T096 [P] 在 `gui/` 中创建 `RoomsPage.qml`
- [ ] T097 在 `src/main.cpp` 中创建 Qt 应用入口, 加载 QML 并注册 C++ 类型

## 阶段 4: 集成
- [ ] T098 将 `LoginViewModel` 与 `MiniEventAdapter` 和 `SQLiteStorage` 连接
- [ ] T099 将 `ChatViewModel` 与 `MiniEventAdapter` 和 `SQLiteStorage` 连接
- [ ] T100 将 ViewModels 暴露给 QML 上下文以实现数据绑定

## 阶段 5: 完善
- [ ] T101 [P] 为 `src/utils/` (如果创建) 中的辅助函数编写单元测试
- [ ] T102 实现文件传输功能, 包括 UI 进度更新
- [ ] T103 [P] 为 QML 视图添加基础样式和主题
- [ ] T104 更新 `README.md` 文件, 包括构建和运行说明