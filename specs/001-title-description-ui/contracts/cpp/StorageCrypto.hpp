#pragma once
#include <string>
#include <vector>

namespace chat {

using Bytes = std::vector<unsigned char>;

struct KeyMaterial {
  std::vector<unsigned char> data; // key bytes
};

class StorageCrypto {
public:
  virtual ~StorageCrypto() = default;
  virtual void setKey(const KeyMaterial& key) = 0;
  virtual void encryptToFile(const std::string& plainPath, const std::string& encPath) = 0;
  virtual void decryptToFile(const std::string& encPath, const std::string& plainPath) = 0;
  virtual Bytes encryptBytes(const Bytes& in) = 0;
  virtual Bytes decryptBytes(const Bytes& in) = 0;
};

} // namespace chat
