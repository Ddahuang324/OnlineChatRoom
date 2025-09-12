````markdown
# 研究: 跨平台现代聊天室

**所在路径(绝对)**: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/research.md

## 目的
解决 spec 中的所有 [需要澄清] 项；验证 MiniEvent 是否满足实时与并发需求；确定存储加密与密钥管理策略；确认多媒体与文件传输策略。

## 待解决的疑问（来自 spec）
- 是否需要端到端加密 (E2EE)?  
  建议: 优先实现静态存储加密 + 传输层加密；如需 E2EE，列为 v2（需要密钥协商与更复杂的 UX）。

- 离线重试阈值 N 的值
  建议: 默认 5 次重试后提示用户（可配置）。

- 审计日志保留期与权限
  已决定: 日志访问不设限制（按用户要求）。在后续实现中仍应记录访问日志以便审计。

- 并发在线用户上限
  目标: ≤ 500。需要通过 MiniEvent 的压力测试确认。研究将包含本地负载测试脚本建议。

- 是否支持移动平台
  建议: 初始目标以桌面为主（macOS/Windows/Linux），移动为可选扩展（Qt for Android/iOS），在阶段 1 确认优先级。

## MiniEvent 能力评估（待执行）
1. 运行 MiniEvent 本地服务，并用并发连接脚本模拟 500 个并发在线用户；测量延迟与丢包率。  
2. 验证 MiniEvent 是否支持二进制多媒体帧和大文件分片上传/断点续传（必要时扩展协议）。

## 存储加密与密钥管理建议
- 静态存储加密: 使用设备密钥或用户口令派生密钥 (PBKDF2/Argon2)，并在本地安全存储（Keychain / Windows Credential Vault / Linux keyring）。
- 如果未来需要 E2EE: 采用公开密钥交换 + per-conversation symmetric keys。

## 媒体与文件传输策略
- 文件分片与断点续传（建议分片 1MB）
- 支持缩略图与转码（音频/视频仅生成低质量预览）
- 最大单文件大小: [需要澄清或在 research 中设置默认，例如 100MB]

## 输出
- 已解决的决策写入: `/Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/research.md`（本文件）
- 如果需要外部依赖或重大设计变更，将在本节记录并通知

````
