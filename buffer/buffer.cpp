#include "buffer.h"

Buffer::Buffer(int bufferSize): m_data(bufferSize, '\0'), m_readPos(0), m_writePos(0) {}

std::size_t Buffer::readableBytes() const {
    return m_writePos - m_readPos;
}

std::size_t Buffer::writeableBytes() const {
    return m_data.size() - m_writePos;
}

std::size_t Buffer::prependableBytes() const {
    return m_readPos;
}

const char* Buffer::beginPtr() const {
    return &*m_data.begin();
}

char* Buffer::beginPtr(){
    return &*m_data.begin();
}

const char* Buffer::peek() const {
    return beginPtr() + m_readPos;
}

const char* Buffer::beginWrite() const {
    return beginPtr() + m_writePos;
}

char* Buffer::beginWrite(){
    return const_cast<char *>(beginPtr() + m_writePos);
}

void Buffer::hasWritten(std::size_t len){
    m_writePos += len;
}

void Buffer::hasReadn(std::size_t len){
    m_readPos += len;
}

void Buffer::clear(){
    m_writePos = 0;
    m_readPos = 0;
    std::fill(m_data.begin(), m_data.end(), '\0');
}

// 向缓冲区写入len个字节数据
void Buffer::append(const char *buff, std::size_t len){
    assert(buff);
    ensureWriteable(len);
    std::copy(buff, buff + len, beginWrite());
    hasWritten(len);
}

void Buffer::append(const std::string &str){
    append(str.data(), str.size());
}

void Buffer::ensureWriteable(std::size_t len){

    if(len > prependableBytes() + writeableBytes()){ 
        m_data.resize(m_writePos + len + 1);
    }
    else if(len > writeableBytes()){
        
        std::size_t readable = readableBytes();
        std::copy(beginPtr() + m_readPos, beginPtr() + m_writePos, beginPtr());
        m_readPos = 0;
        m_writePos = readable;
        assert(readableBytes() == m_writePos);
    }

    assert(writeableBytes() >= len);
}

/**
 * 将缓冲区读取的数据写入文件fd中 
 * 若发生错误则将errno保存在saveErrno中 
 * 返回成功写入到文件的字节数
 * */ 
ssize_t Buffer::writeFd(int fd, int &saveErrno){
    std::size_t readSize = readableBytes();

    ssize_t len = write(fd, peek(), readSize);
    if(len < 0){
        saveErrno = errno;
        return len;
    }
    hasReadn(static_cast<std::size_t>(len));
    return len; 
}

ssize_t Buffer::readFd(int fd, int &saveErrno){\
    // 为保证数据能全部写入缓冲区 需准备一个临时的字节数组 将多余的部分读入临时数组中
    char buff[65536] = {0};
    struct iovec iov[2];
    std::size_t writeBytes = writeableBytes();
    
    iov[0].iov_base = beginWrite();
    iov[0].iov_len = writeBytes;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t len = readv(fd, iov, 2);
    if(len < 0){
        saveErrno = errno;
    }
    else if(static_cast<std::size_t>(len) <= writeBytes){
        hasWritten(static_cast<std::size_t>(len));
    }
    else{
        m_writePos = m_data.size();
        append(buff, static_cast<std::size_t>(len) - writeBytes);
    }
    return len;
}

std::string Buffer::bufferToString(){
    std::string res(peek(), readableBytes());
    // clear();
    return res;
}



