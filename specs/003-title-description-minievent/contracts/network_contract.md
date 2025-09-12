# contracts/network_contract.md

目标：定义 MiniEvent 适配器对系统暴露的网络契约（接口与行为）

接口 (C++ header-like sketch):

struct ConnectionParams {
  std::string host;
  uint16_t port;
  int timeoutMs;
};

struct SerializedMessage {
  std::string id;
  std::string roomId;
  std::string senderId;
  int64_t timestamp;
  std::string payload; // JSON or binary
};

struct FileChunk {
  std::string fileId;
  uint64_t offset;
  std::vector<uint8_t> bytes;
  bool last;
};

enum class ConnectionEvent { Connected, Disconnected, Reconnecting, Failed };

class MiniEventAdapter {
public:
  virtual ~MiniEventAdapter() = default;
  virtual bool connect(const ConnectionParams& params) = 0;
  virtual void disconnect() = 0;
  virtual bool sendMessage(const SerializedMessage& msg) = 0;
  virtual bool sendFileChunk(const FileChunk& chunk) = 0;
  virtual void setMessageHandler(std::function<void(const SerializedMessage&)> cb) = 0;
  virtual void setConnectionHandler(std::function<void(ConnectionEvent)> cb) = 0;
};

行为契约:
- sendMessage 返回 true 表示已成功提交到底层发送队列，不代表已被对端接收
- messageHandler 在接收到完整消息或文件完整上传后由底层线程回调，回调必须在适配层转换为主线程可消费的信号
- 必须支持主动断连与自动重连，并通过 connectionHandler 通知 ViewModel
