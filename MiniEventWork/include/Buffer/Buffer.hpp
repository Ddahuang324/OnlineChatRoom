#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cassert>
#include <cstring>



class Buffer {
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;
    // Maximum buffer size (1MB)
    static const size_t kMaxSize = 1024 * 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readIndex_(kCheapPrepend),
          writeIndex_(kCheapPrepend) {}

    // 可读、可写、预留空间大小
    size_t readableBytes() const { return writeIndex_ - readIndex_; }
    size_t writableBytes() const { return buffer_.size() - writeIndex_; }
    size_t prependableBytes() const { return readIndex_; }

    // 返回可读数据的起始位置
    const char* peek() const { return begin() + readIndex_; }

    // 返回可写缓冲区的起始位置
    char* beginWrite() { return begin() + writeIndex_; }
    const char* beginWrite() const { return begin() + writeIndex_; }

    // 从缓冲区中“取出”数据，实际上只是移动指针
    void retrieve(size_t len) {
        assert(len <= readableBytes());
        if (len < readableBytes()) {
            readIndex_ += len;
        } else {
            retrieveAll();
        }
    }

    void retrieveAll() {
        readIndex_ = kCheapPrepend;
        writeIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len) {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    // 向缓冲区追加数据
    void append(const char* data, size_t len) {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }

    // convenience overload
    void append(const std::string& data) { append(data.data(), data.size()); }

    // 确保有足够的可写空间
    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    // 更新写指针
    void hasWritten(size_t len) {
        assert(len <= writableBytes());
        writeIndex_ += len;
    }

    // 从文件描述符读取数据
    ssize_t readFd(int fd, int* savedErrno);

    // 返回 peek 指向的数据片段的字符串表示（不修改索引）
    std::string peekAsString(size_t len) const {
        assert(len <= readableBytes());
        return std::string(peek(), len);
    }

    // 在可读数据中查找子串，返回指针（指向首次出现位置），未找到返回 nullptr
    const char* find_str(const char* needle) const {
        const char* start = peek();
        const char* end = begin() + writeIndex_;
        size_t needle_len = ::strlen(needle);
        if (needle_len == 0) return start; // 空串返回起始
        for (const char* p = start; p + needle_len <= end; ++p) {
            if (std::memcmp(p, needle, needle_len) == 0) {
                return p;
            }
        }
        return nullptr;
    }

    // Find CRLF (\r\n) and return the position offset from peek()
    size_t findCRLF() const {
        const char* crlf = find_str("\r\n");
        return crlf ? (crlf - peek() + 2) : 0; // +2 to include \r\n
    }

private:
    char* begin() { return &*buffer_.begin(); }
    const char* begin() const { return &*buffer_.begin(); }

    void makeSpace(size_t len);

private:
    std::vector<char> buffer_;
    size_t readIndex_;
    size_t writeIndex_;
};

