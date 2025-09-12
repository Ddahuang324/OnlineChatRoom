````markdown
# UI 设计: MVVM + 现代动画与交互

**所在路径(绝对)**: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/ui-design.md

## 架构决定
- 采用 MVVM 模式：View(QML) <-> ViewModel(C++) <-> Model(Core services)
- ViewModel 通过 Q_PROPERTY 和 Q_INVOKABLE 暴露给 QML，所有异步事件使用 Qt 的信号/槽并以 queued connection 送回 UI 线程

## 页面与交互概览
- 会话列表 (ConversationList)
  - 核心交互: 滑动刷新、长按显示会话操作菜单（置顶/静音/删除）、会话快速预览动画
  - 动画: 使用 QML 的 State/Transition 与 Behavior，列表项进入/离开有平滑高度与 opacity 动画

- 聊天视图 (ChatView)
  - 核心交互: 消息流入动画（自下而上淡入+位移动画）、发送气泡的弹性反馈、图片/视频预览弹窗（带缩放动画）
  - 输入区域支持富文本占位与附件预览（文件拖放或相机拍摄）
  - 滚动行为: 新消息自动滚底，用户上滑查看历史时保留相对偏移并阻止自动滚动

- 文件/多媒体上传体验
  - 上传前展示缩略图与文件信息，上传时展示进度条与取消按钮
  - 分片上传失败时展示重试动作与局部回退动画

## 动画与响应性原则
- 使用 60fps 为目标，避免阻塞 UI 线程
- 动画应为可配置（启用/禁用）以支持低性能设备
- 交互反馈应即时（<100ms 感知延迟），网络延迟由 placeholder + skeleton UI 覆盖

## ViewModel 设计（职责）
- ConversationListViewModel
  - 属性: conversations: QVariantList (每项包含 id, title, lastMsg, unreadCount, avatar)
  - 方法: refresh(), loadMore(beforeEpochMs), togglePin(conversationId), setMuted(conversationId, bool)
  - 信号: conversationUpdated(conversationId), conversationRemoved(conversationId)

- ChatViewModel
  - 属性: messages: QVariantList (Message objects), isLoadingHistory: bool, sendingQueue: QVariantList
  - 方法: sendText(conversationId, text), sendFile(conversationId, filePath), loadHistory(conversationId, beforeEpochMs), retryMessage(localId)
  - 信号: messageSent(messageId), messageFailed(localId), messageReceived(messageId)

- PresenceViewModel
  - 属性: presenceMap: QVariantMap userId->state
  - 方法: fetchPresence(userIds), subscribe(userId)
  - 信号: presenceChanged(userId, state)

## 数据绑定与序列化
- ViewModel 输出的数据为 QVariants（map/list），Service 层提供 JSON-serializable DTOs
- 时间字段在 UI 使用 ISO8601 字符串显示，内部以 epoch_ms 存储

## 性能/内存控制
- 消息虚拟化：仅渲染可视区域的消息项（使用 ListView 的 delegate + cacheBuffer）
- 大文件预览使用临时缩略图文件，避免一次性加载全部到内存

## 可测试点（UI 层）
- 会话加载/分页的视觉回归测试
- 新消息动画触发与不阻塞交互的自动化测试（脚本化截图+时间断言）
- 文件上传失败/重试的 UI 行为测试

````
