#ifndef __BUFFER__H__
#define __BUFFER__H__

#include <vector>
#include <atomic>
#include <string>
#include <cassert>

#include <unistd.h>
#include <sys/uio.h>

class Buffer{

public:
    explicit Buffer(int bufferSize = 1024);

    std::size_t readableBytes() const;  // 可读的字节数
    std::size_t writeableBytes() const; // 缓冲区尾端空闲的字节数
    std::size_t prependableBytes() const;   // 缓冲区尾端空闲的字节数

    const char* peek() const;   // 读位置指针
    const char* beginWrite() const; // 写位置指针
    char* beginWrite();

    void hasWritten(std::size_t len);  // 增加writePos
    void hasReadn(std::size_t len);     // 移动readPos
    void clear();   // 清空缓冲区

    void append(const char *buff, std::size_t len); // 向缓冲区写入len个字节数据
    void append(const std::string &str);

    ssize_t writeFd(int fd, int &saveErrno);   // 将缓冲区的数据写入文件fd中 若发生错误则将errno保存在saveErrno中 返回写入的字节数
    ssize_t readFd(int fd, int &saveErrno);    // 将文件fd中的内容读入缓冲区中

    std::string bufferToString();

private:
    const char* beginPtr() const;   // 缓冲区起始位置指针
    char* beginPtr();
    
    void ensureWriteable(std::size_t len);  // 检查剩余空间是否能写

private:
    std::vector<char> m_data;
    std::atomic<std::size_t> m_readPos;
    std::atomic<std::size_t> m_writePos;
};


#endif  //!__BUFFER__H__