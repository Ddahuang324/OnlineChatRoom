#include "../include/Buffer/Buffer.hpp"
#include <sys/uio.h>


   
ssize_t Buffer::readFd(int fd, int* savedErrno) {
    //准备一个栈缓冲区
    // 定义一个大小为65536的字符数组extrabuf，用于存储额外的数据
    char extrabuf[65536];
    // 定义一个包含两个iovec结构的数组iov，用于在读写操作中分散或聚集内存区域
    struct iovec vec[2];
    // 获取当前缓冲区可写入的字节数，并存储在writable变量中
    const size_t writable = writableBytes();

    //第一块指向内部Buffer的可写空间
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;
//第二块指向extrabuf的可读空间(栈空间)
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    //使用readv 一次性从fd 读数据到多个缓冲区
    // 根据可写数据长度确定向量数量，如果可写数据长度小于额外缓冲区大小，则使用2个向量，否则使用1个向量
    const int iovcnt = (writable < sizeof(extrabuf))? 2 : 1;
    const ssize_t n = readv(fd, vec, iovcnt);

    if (n < 0){
        *savedErrno = errno;
        return -1;
    }else if (static_cast<size_t>(n) <= writable){
        //如果读到的数据长度小于等于可写数据长度，则只占用内部buffer
        writeIndex_ += n;
    }else{
        //如果读到的数据长度大于可写数据长度，则将读到的数据追加到栈缓冲区
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    } 
    return n;
};

void Buffer::makeSpace(size_t len) {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
        // 空间不足，需要扩容，检查是否会超过最大值：kMaxSize
        size_t newSize = writeIndex_ + len;
        if (newSize > kMaxSize) {
            // 超过最大限制：拒绝扩容，尽可能抛出或截断
            // 这里选择将 len 截断为允许的最大可写大小
            size_t allowed = (kMaxSize > writeIndex_) ? (kMaxSize - writeIndex_) : 0;
            if (allowed == 0) {
                // 无法再写入任何数据
                throw std::length_error("Buffer maximum size exceeded (1MB)");
            }
            buffer_.resize(kMaxSize);
        } else {
            buffer_.resize(newSize);
        }
    } else {
        // 空间足够，移动数据
        size_t readable = readableBytes();
        std::copy(begin() + readIndex_,
                  begin() + writeIndex_,
                  begin() + kCheapPrepend);
        readIndex_ = kCheapPrepend;
        writeIndex_ = readIndex_ + readable;
        assert(readable == readableBytes());
    }
}
