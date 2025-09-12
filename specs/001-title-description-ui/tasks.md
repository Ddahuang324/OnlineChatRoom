# 任务清单: 标题与描述 UI

功能目录: `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui`
可用文档: `plan.md`, `research.md`, `data-model.md`, `contracts/openapi.yaml`, `quickstart.md`

优先级与约定:
- [P] 标记表示可并行运行（任务修改不同文件）
- 所有路径使用绝对路径以便自动 agent 运行
- 编号规则: T001, T002 ...

## 设置阶段
- T001  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui` 确保项目结构存在: `src/`, `docs/`, `tests/`, `contracts/`（已创建）
- T002  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/CMakeLists.txt` 初始化 CMake 项目并配置 C++17/20、Qt (Qt Quick) 与 MiniEvent 依赖
- T003  [P] 在仓库根（`/Users/dahuang/Desktop/编程/项目/OnlineChat`）配置代码检查与格式化: `.clang-format`, `.clang-tidy`, CI workflow (`.github/workflows/ci.yml`)（路径: 见各文件）

## 测试优先 (TDD) — 契约与基础模型（必须在实现前完成）
- T004  [P] 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/tests/contract/test_openapi_contract.cpp` 为 `contracts/openapi.yaml` 编写契约测试（检查所有端点的请求/响应约束）

### 从 data-model.md 生成的模型任务（均标记为 [P]，可并行）
- T005  [P] 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/models/User.hpp|.cpp` 实现 `User` 模型（字段: user_id, display_name, avatar_url, presence, devices）
- T006  [P] 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/models/Conversation.hpp|.cpp` 实现 `Conversation` 模型
- T007  [P] 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/models/Message.hpp|.cpp` 实现 `Message` 模型
- T008  [P] 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/models/FileMeta.hpp|.cpp` 实现 `FileMeta` 模型
- T009  [P] 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/models/Group.hpp|.cpp` 实现 `Group` 模型

## 核心服务与适配层（模型完成后实现）
- T010  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/core/SqlStorage.hpp|.cpp` 实现 SqlStorage（init/putMessage/getMessages/tx）
- T011  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/core/MiniEventAdapter.hpp|.cpp` 实现 MiniEventAdapter（connect/disconnect/subscribe/sendMessage/onMessage）
- T012  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/core/MessageService.hpp|.cpp` 实现 MessageService（enqueueOutgoing/listRecent/markDelivered/markRead）
- T013  [P] 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/core/FileUploader.hpp|.cpp` 实现 FileUploader 接口（分片上传策略将在 `research.md` 明确）

## API 端点实现（服务准备好后）
（每个端点为独立实现任务；同一文件内的多个端点需顺序执行）
- T014  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/api/auth.cpp` 实现 `POST /auth/login`（验证、返回 token + user）
- T015  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/api/conversations.cpp` 实现 `GET /conversations`
- T016  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/api/messages_get.cpp` 实现 `GET /conversations/{conversation_id}/messages`
- T017  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/api/messages_post.cpp` 实现 `POST /conversations/{conversation_id}/messages`
- T018  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/api/files_upload.cpp` 实现 `POST /files/upload`（创建 FileMeta、返回 file_id）
- T019  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/api/files_upload_chunk.cpp` 实现 `PUT /files/upload/chunk`（接收二进制分片，记录 idx）

## 集成任务（核心实现后）
- T020  将 `MessageService` 与 `SqlStorage` 与 `MiniEventAdapter` 集成并在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/tests/integration/test_message_flow.cpp` 编写集成测试
- T021  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/middleware/auth.hpp|.cpp` 实现认证中间件并测验
- T022  [P] 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/src/logging/` 集成结构化日志与基础指标（metrics）

## 完善与发布准备
- T023  [P] 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/tests/unit/` 为模型和服务编写单元测试
- T024  在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/tests/perf/` 运行性能测试（目标: 消息延迟 <200ms p95，基于本地 MiniEvent 测试服务器）
- T025  [P] 更新文档: `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/docs/api.md`、`quickstart.md`

## 依赖关系概要
- 设置: T001-T003 先于一切
- 契约测试: T004 必须在端点实现 (T014-T019) 之前完成
- 模型: T005-T009 先于核心服务 T010-T013
- 服务: T010-T013 先于端点实现 T014-T019
- 核心与端点完成后进行集成 T020-T022，然后完善 T023-T025

## 并行执行示例
- 可同时并行运行 (不同文件、无依赖): T003, T004, T005, T006, T007, T008, T009, T013, T022, T023, T025

示例 Task agent 命令 (复制执行):
```bash
# 运行契约测试 (并行组)
agent run --task T004

# 在不同终端并行生成模型
agent run --task T005 & agent run --task T006 & agent run --task T007
```

## 验证清单
- [ ] 所有契约文件均有对应的契约测试 (contracts/openapi.yaml -> T004)
- [ ] data-model 中的每个实体对应模型任务 (User, Conversation, Message, FileMeta, Group)
- [ ] 测试在实现前存在并最初失败（TDD）
- [ ] 并行任务不在同一文件上冲突

## 下步
1. 完成 T004（编写并运行契约测试）→ 记录失败输出
2. 并行完成 T005-T009（模型）与 T003（代码检查）
3. 按依赖顺序实现服务与端点
````markdown
# 任务清单: 跨平台现代聊天室 (TDD 优先 / 契约优先)

**输入**: plan.md, research.md, data-model.md, contracts/openapi.yaml
**所在路径(绝对)**: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/tasks.md

## 执行规则
- 测试必须先于实现（TDD）
- 契约测试优先：每个 contracts/ 文件对应至少一个契约测试
- 使用 [P] 标注可并行任务（不同文件、无依赖）

## 阶段 3.1: 设置
- T001: 创建项目源目录与 CMake 基础文件（/Users/dahuang/Desktop/编程/项目/OnlineChat/src/, CMakeLists.txt）
- T002: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/.vscode/tasks.json` 添加构建/调试任务（可选）
- T003 [P]: 配置代码风格与静态分析（clang-format, clang-tidy），并添加到 CI 配置文件（.github/workflows/）

## 阶段 3.2: 契约测试与集成测试 (TDD) ⚠️ 必须先完成
- T004 [P]: 为 `contracts/openapi.yaml` 中的 `POST /auth/login` 创建契约测试文件 `/Users/dahuang/Desktop/编程/项目/OnlineChat/tests/contract/test_auth_login.cpp`
- T005 [P]: 为 `GET /conversations` 创建契约测试 `/Users/dahuang/Desktop/编程/项目/OnlineChat/tests/contract/test_conversations_list.cpp`
- T006 [P]: 为 `GET /conversations/{conversation_id}/messages` 创建契约测试 `/Users/dahuang/Desktop/编程/项目/OnlineChat/tests/contract/test_conversation_messages.cpp`
- T007 [P]: 为 `POST /conversations/{conversation_id}/messages` 创建契约测试 `/Users/dahuang/Desktop/编程/项目/OnlineChat/tests/contract/test_post_message.cpp`
- T008 [P]: 为 `/files/upload` 创建契约测试 `/Users/dahuang/Desktop/编程/项目/OnlineChat/tests/contract/test_file_upload.cpp`（应覆盖分片/断点续传场景的占位测试）
- T009 [P]: 为 MiniEvent 协议层创建集成测试 `/Users/dahuang/Desktop/编程/项目/OnlineChat/tests/integration/test_minievent_protocol.cpp`（模拟 2 个客户端的消息交换）

## 阶段 3.3: 数据模型与核心服务（在契约测试存在且失败后实现）
- T010: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/core/models/` 创建实体模型文件（User, Conversation, Message, FileMeta, Group）
- T011: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/core/storage/` 实现本地存储适配（SQLite + 加密层），文件路径示例 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/core/storage/SqlStorage.cpp`
- T012: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/core/services/` 实现消息服务（MessageService），负责消息入队、持久化、状态更新
- T013: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/core/crypto/` 实现静态存储加密模块（Key derivation + encrypt/decrypt），示例文件 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/core/crypto/StorageCrypto.cpp`

## 阶段 3.4: MiniEvent 适配层与网络集成
- T014 [P]: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/core/net/` 创建 MiniEvent 适配器接口 `MiniEventAdapter`（抽象）
- T015: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/core/net/impl/` 实现具体适配器 `MiniEventAdapterImpl.cpp`，处理连接、订阅、消息发送/接收、心跳
- T016: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/tests/integration/test_minievent_adapter.cpp` 编写适配器集成测试（需 MiniEvent 本地服务运行）

## 阶段 3.5: 文件与多媒体传输
- T017 [P]: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/media/` 实现文件分片上传/断点续传逻辑（FileUploader）
- T018 [P]: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/media/` 实现多媒体处理（缩略图、简单转码接口）
- T019: 将文件元数据持久化到 storage（见 T011），并确保文件在磁盘以加密形式存储
- T020: 为文件上传/下载实现契约测试与集成测试（对应 T008）

## 阶段 3.6: UI (Qt/QML) 骨架与交互
- T021: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/app/` 创建 Qt/CMake 项目骨架与主窗口（Main.qml + main.cpp）
- T022 [P]: 创建基本界面页面文件 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/app/qml/ConversationList.qml` 和 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/app/qml/ChatView.qml`
- T023: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/app/` 实现前端与后端 glue（使用 Qt 的信号槽或 QML 注册 C++ 对象），例如 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/app/AppBackend.cpp`
- T024: 为 UI 编写集成测试脚本（手动或自动化），示例路径 `/Users/dahuang/Desktop/编程/项目/OnlineChat/tests/ui/test_smoke_ui.md`

## 阶段 3.7: 离线支持与同步
- T025: 实现在网络中断时消息队列缓存（本地 queue 实现），文件 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/core/offline/OfflineQueue.cpp`
- T026: 实现恢复连接后的消息同步逻辑，确保顺序与重复消息去重

## 阶段 3.8: 日志、审计与权限（注意: 日志访问不设限制）
- T027: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/src/core/log/` 增加结构化日志记录（可选向外发送或写本地文件）
- T028: 为关键操作（群组管理、文件删除）记录审计事件并保存到审计日志存储 `/Users/dahuang/Desktop/编程/项目/OnlineChat/logs/audit.log`

## 阶段 3.9: 性能与压力测试
- T029 [P]: 创建 MiniEvent 本地压力测试脚本 `/Users/dahuang/Desktop/编程/项目/OnlineChat/tools/load_test/run_500_users.sh`，模拟 ≤500 并发在线用户并测量延迟
- T030: 在 `/Users/dahuang/Desktop/编程/项目/OnlineChat/tests/perf/test_end_to_end_latency.md` 定义性能验证步骤和成功标准（例如 p95 < 200ms）

## 阶段 3.10: 文档与快速上手
- T031 [P]: 更新 `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/quickstart.md` 添加确切构建与运行命令
- T032: 编写部署说明与运维注意事项 `/Users/dahuang/Desktop/编程/项目/OnlineChat/docs/deploy.md`

## 阶段 3.11: CI 与质量门
- T033: 在 `.github/workflows/ci.yml` 添加 CI 流程：构建、运行契约测试、运行静态检查
- T034: 在 CI 中加入压力测试（可选，分支触发）

---

## 依赖关系（摘要）
- 契约测试 (T004-T009) 必须先于相应实现 (T010-T016, T017-T019)
- 模型 (T010) 阻塞存储实现 (T011) 与服务 (T012)
- MiniEvent 适配器 (T014-T016) 阻塞 UI 实时功能 (T021-T024)

## 并行执行示例
- 同时运行: T004,T005,T006,T007,T008（契约测试）
- 同时运行: T017,T018,T031（多媒体与 quickstart 文档）

````
