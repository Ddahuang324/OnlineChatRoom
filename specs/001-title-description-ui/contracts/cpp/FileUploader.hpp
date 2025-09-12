#pragma once
#include <functional>
#include <string>

namespace chat {

using ProgressCb = std::function<void(long long sentBytes, long long totalBytes)>;

struct UploadResult {
  bool ok{false};
  std::string fileId;
  int httpStatus{0};
};

struct FileMeta {
  std::string filename;
  std::string mimeType;
  long long sizeBytes{0};
};

class Readable {
public:
  virtual ~Readable() = default;
  virtual long long read(char* buf, long long maxBytes) = 0;
};

class FileUploader {
public:
  virtual ~FileUploader() = default;
  virtual void configureChunkSize(size_t bytes) = 0; // default 1MB
  virtual UploadResult upload(const FileMeta& meta, Readable& reader, ProgressCb cb) = 0;
  virtual UploadResult resume(const std::string& fileId, Readable& reader, ProgressCb cb) = 0;
};

} // namespace chat
