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
[ ] T021 定义 MiniEventAdapter 抽象 (补充状态码)
  文件: include/net/MiniEventAdapter.h
  枚举: NetStatus { Ok, NotConnected, Timeout, QueueFull, SerializeError, InternalError }

[ ] T022 [P] Adapter 实现骨架头
  文件: src/net/MiniEventAdapterImpl.h
  成员: std::thread recvThread_; std::thread sendThread_; std::mutex sendMutex_; std::condition_variable sendCv_; std::queue<SerializedMessage> sendQueue_; std::atomic<bool> connected_{false}; std::atomic<bool> stop_{false}; 回调 std::function<void(... )> msgCb_, connCb_;

[ ] T023 连接流程实现 connect()/disconnect()
  文件: src/net/MiniEventAdapterImpl.cpp
  指数退避: base=500ms factor=2 max=30s attempts<=8
  错误: DNS失败 / 超时 → NetStatus::Timeout

[ ] T024 [P] 发送线程循环
  文件: src/net/MiniEventAdapterImpl.cpp
  逻辑: while(!stop_) wait(sendQueue_) → serialize → MiniEvent send
  队列满判定: size>MAX_QUEUE(1024) → 返回 QueueFull

[ ] T025 接收线程与回调主线程封送
  文件: src/net/MiniEventAdapterImpl.cpp
  使用: QMetaObject::invokeMethod(nullptr, lambda, Qt::QueuedConnection)
  事件: Connected/Disconnected/Reconnecting

[ ] T026 [P] sendFileChunk 基线实现
  文件: src/net/MiniEventAdapterImpl.cpp
  校验: chunk.bytes.size <= 64*1024; offset 对齐

[ ] T027 超时/错误映射策略文档
  文件: docs/network-error-mapping.md
  内容: MiniEvent errno → NetStatus 映射表

[ ] T028 Adapter 单元测试
  文件: tests/net/test_mini_event_adapter.cpp
  场景: 成功连接/错误端口/消息往返/队列满

-------------------------------------------------
阶段 5: ViewModel 层 (Login / Chat)
-------------------------------------------------
[ ] T029 LoginViewModel 头文件
  文件: src/viewmodel/LoginViewModel.h
  成员: Q_OBJECT; Storage* storage_; MiniEventAdapter* net_; QThread workerThread_; bool loggingIn_;
  信号: loginSucceeded(), loginFailed(QString)

[ ] T030 [P] LoginViewModel 实现 login/logout
  文件: src/viewmodel/LoginViewModel.cpp
  逻辑: 移交到 workerThread 执行 net_->connect → 回主线程发信号

[ ] T031 ChatViewModel 头文件
  文件: src/viewmodel/ChatViewModel.h
  成员: Storage* storage_; MiniEventAdapter* net_; QUuidGenerator helper; signal messageReceived(...)

[ ] T032 sendText 实现
  文件: src/viewmodel/ChatViewModel.cpp
  步骤: 生成 UUID → 构建 MessageRecord(state=Pending) → storage_->saveMessage → net_->sendMessage → 更新状态

[ ] T033 [P] onIncomingMessage & message 保存
  文件: src/viewmodel/ChatViewModel.cpp
  验收: 收到后发射 messageReceived(QVariantMap)

[ ] T034 sendFile 基础实现 (调度 FileTransferManager)
  文件: src/viewmodel/ChatViewModel.cpp
  依赖: T040

[ ] T035 ViewModel 单元测试
  文件: tests/viewmodel/test_login.cpp / test_chat.cpp
  使用 MockStorage / MockMiniEventAdapter

-------------------------------------------------
阶段 6: 文件传输子系统
-------------------------------------------------
[ ] T036 FileTransferManager 头文件
  文件: src/transfer/FileTransferManager.h
  成员: struct UploadCtx{ std::string fileId; uint64_t sent; uint64_t total; int retries; }; Storage* storage_; MiniEventAdapter* net_; QThreadPool pool_; size_t concurrency_=3;

[ ] T037 [P] FileTransferManager.cpp 上传调度 uploadFile
  分片: 64KiB; 读取 → 推线程池任务 → net_->sendFileChunk

[ ] T038 onChunkAck 处理与进度保存
  文件: src/transfer/FileTransferManager.cpp
  调用: storage_->saveFileProgress(fileId, offset)

[ ] T039 [P] resumeUpload 断点续传
  文件: src/transfer/FileTransferManager.cpp
  逻辑: loadFileProgress → 继续 offset 对齐的下一片

[ ] T040 与 ChatViewModel 集成 (注入 manager)
  文件: src/viewmodel/ChatViewModel.cpp / .h

[ ] T041 传输重试与指数退避策略
  文件: src/transfer/FileTransferManager.cpp
  算法: attemptDelay= base*2^retries (base=300ms, max=5s, retries<=3)

[ ] T042 单元测试文件传输
  文件: tests/transfer/test_file_transfer.cpp
  场景: 正常上传 / 中断恢复 / 重试上限

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
