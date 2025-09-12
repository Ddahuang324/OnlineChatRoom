# Data Model

数据结构（C++ 结构体/类示例）:

- UserRecord
  - std::string id
  - std::string displayName
  - std::string avatarPath
  - enum Status { Offline, Online, Typing }

- MessageRecord
  - std::string id
  - std::string roomId
  - std::string senderId
  - int64_t localTimestamp
  - int64_t serverTimestamp (optional)
  - enum Type { Text, File }
  - std::string content (for Text)
  - FileMeta fileMeta (for File)
  - enum State { Pending, Sent, Delivered, Failed }

- FileMeta
  - std::string fileName
  - int64_t size
  - std::string mimeType
  - std::string sha256

数据库表 (SQLite) 示例:
- users(id TEXT PRIMARY KEY, display_name TEXT, avatar TEXT)
- rooms(id TEXT PRIMARY KEY, name TEXT, is_public INTEGER)
- messages(id TEXT PRIMARY KEY, room_id TEXT, sender_id TEXT, local_ts INTEGER, server_ts INTEGER, type INTEGER, content TEXT, file_meta JSON, state INTEGER)
