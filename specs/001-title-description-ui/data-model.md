````markdown
# 数据模型草案（强化：类型/约束/不变量）

**所在路径(绝对)**: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/data-model.md

通用约束
- ID 格式: UUID v4 字符串（长度 36，正则 `^[0-9a-fA-F-]{36}$`）
- 时间戳: 64-bit epoch_ms（long long），所有存储使用 UTC
- 文本编码: UTF-8；最大长度在字段中明确
- 枚举取值固定；未知值一律拒绝

## 实体与字段

用户 (User)
- user_id: string(36) PK, UUID v4, NOT NULL, UNIQUE
- display_name: string(1..64), NOT NULL
- avatar_url: string(0..256), 可为空
- presence: enum {online, offline, away}, 默认 offline
- devices: JSON 数组（设备会话的摘要），可为空

会话 (Conversation)
- conversation_id: string(36) PK, UUID v4, NOT NULL
- type: enum {direct, group}, NOT NULL
- member_ids: JSON 数组<string(36)>, 大小 2..500（并发目标对齐）
- last_activity_at: epoch_ms, NOT NULL
- unread_count: int(0..2^31-1), 默认 0
不变量:
- type=direct 时 member_ids.size()==2
- type=group 时 member_ids.size()>=3

消息 (Message)
- message_id: string(36) PK, UUID v4, NOT NULL
- conversation_id: string(36) FK→Conversation, NOT NULL
- sender_id: string(36) FK→User, NOT NULL
- content_type: enum {text, image, file, audio, video}, NOT NULL
- content_ref: string(0..2048), NOT NULL（文本内容或媒体/file 引用）
- created_at: epoch_ms, NOT NULL
- delivered_at: epoch_ms, 可为空（<= now && >= created_at）
- read_at: epoch_ms, 可为空（>= delivered_at）
- status: enum {queued, sent, delivered, read, failed}, NOT NULL
不变量:
- status 顺序必须单调不降: queued→sent→delivered→read/failed

文件元数据 (FileMeta)
- file_id: string(36) PK, UUID v4, NOT NULL
- owner_id: string(36) FK→User, NOT NULL
- filename: string(1..255), NOT NULL
- mime_type: string(1..128), NOT NULL
- size_bytes: int64(0..?), NOT NULL（默认上限: 100MB，需在 research 确认）
- storage_path: string(1..512), NOT NULL（磁盘加密存储路径）
- uploaded_at: epoch_ms, NOT NULL
不变量:
- storage_path 指向受静态加密保护的文件

群组 (Group)
- group_id: string(36) PK, UUID v4, NOT NULL
- name: string(1..64), NOT NULL
- avatar_url: string(0..256), 可为空
- admin_ids: JSON 数组<string(36)>, NOT NULL, size>=1

## 关系与外键
- Message.conversation_id → Conversation.conversation_id (ON DELETE CASCADE)
- Message.sender_id → User.user_id (ON DELETE RESTRICT)
- FileMeta.owner_id → User.user_id (ON DELETE SET NULL/RESTRICT，需选一)

## 索引
- Message(conversation_id, created_at DESC)
- Conversation(last_activity_at DESC)
- Message(status) 局部索引（仅 queued/sent）

## 验证规则（入库前）
- content_type=text 时 content_ref.length<=5000（文本上限，防止超长）
- 文件 size_bytes<=MAX_FILE_SIZE（默认 100MB，可配置）
- 时间戳不得早于 2000-01-01 且不得晚于 now()+5min

## 序列化规范
- ID 一律小写 UUID 字符串
- 时间戳对外 API 使用 ISO8601 或 epoch_ms（OpenAPI 中声明）

## 数据迁移注意
- 初始 schema 版本 v1；后续变更需附带迁移脚本与回滚策略

````
