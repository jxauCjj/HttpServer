#include "httpconn.h"

std::atomic<int> HttpConn::g_userCount = 0; // 初始化
const char *HttpConn::g_srcDir = "./resources"; // 资源文件目录

HttpConn::HttpConn() {
    m_fd = 1;
    m_isClose = true;
    memset(&m_addr, 0x00, sizeof(m_addr));
}

void HttpConn::init(int fd, struct sockaddr_in &addr){
    assert(fd >= 0);
    m_fd = fd;
    m_addr = addr;
    ++g_userCount;
    m_isClose = false;
    m_rdBuff.clear();
    m_wrBuff.clear();

    LOG_INFO("Client[%d] (%s:%d) Connect, UserCount:%d", m_fd, getIP(), getPort(), static_cast<int>(g_userCount));
}

void HttpConn::closeConn() {
    
    if(!m_isClose){
        close(m_fd);
        --g_userCount;
        m_isClose = true;
        m_response.unmapFile();

        LOG_INFO("Client[%d] (%s:%d) Close, UserCount:%d", m_fd, getIP(), getPort(), static_cast<int>(g_userCount));
    }
    else{
        LOG_WARN("Client[%d] (%s:%d) has been Closed, UserCount:%d", m_fd, getIP(), getPort(), static_cast<int>(g_userCount));
    }
}

int HttpConn::getFd() const {
    return m_fd;
}

int HttpConn::getPort() const {
    return ntohs(m_addr.sin_port);
}

const char* HttpConn::getIP() const {
    return inet_ntoa(m_addr.sin_addr);
}

// 读取数据
ssize_t HttpConn::read(int &saveErrno){
    
    ssize_t len = -1;
    len = m_rdBuff.readFd(m_fd, saveErrno);
    // if(len > 0){
    //     std::string msg = m_rdBuff.bufferToString(); 
    //     m_wrBuff.append(msg); // Debug 回声服务器

    //     LOG_INFO("Receive Client [%d] (%s:%d) Message: %s", m_fd, getIP(), getPort(), msg.c_str());
    // }
    return len;
}

// 发送数据
ssize_t HttpConn::write(int &saveErrno){
    ssize_t len = -1;
    
    while(toWriteBytes() > 0){
        len = writev(m_fd, m_iov, m_iovCnt);

        if(len <= 0) {
            saveErrno = errno;
            return len;
        }
        else if(static_cast<std::size_t>(len) <= m_iov[0].iov_len) {
            m_iov[0].iov_base = (uint8_t *)m_iov[0].iov_base + len;
            m_iov[0].iov_len -= len;
            m_wrBuff.hasReadn(len);
        }
        else {
            m_iov[1].iov_base = (uint8_t *)m_iov[1].iov_base + (len - m_iov[0].iov_len);
            m_iov[1].iov_len -= (len - m_iov[0].iov_len);

            if(m_iov[0].iov_len > 0) {
                m_iov[0].iov_len = 0;
                m_wrBuff.clear();
            }
        }

        // LOG_DEBUG("Send File %d bytes, Remain %d bytes", len, toWriteBytes());
    }
    
    // if(len > 0){
    //     LOG_INFO("Send Client [%d] (%s:%d) Message", m_fd, getIP(), getPort());
    // }
    return len;
}

// 解析http请求
bool HttpConn::process() {

    m_request.init();

    if(!m_request.parse(m_rdBuff)){  // 解析成功则发送响应数据 失败则说明数据包不完整
        return false;
    }
    m_response.init(m_request.statusCode(), m_request.path(), "./resources", m_request.isKeepAlive());
    m_response.makeResponse(m_wrBuff);  // 组装响应报文

    // 根据组装好的报文确定发送的数据位置和大小
    m_iov[0].iov_base = const_cast<char *>(m_wrBuff.peek());
    m_iov[0].iov_len = m_wrBuff.readableBytes();
    m_iovCnt = 1;

    if(m_response.fileLen() > 0 && m_response.fileAddr()) {
        m_iov[1].iov_base = m_response.fileAddr();
        m_iov[1].iov_len = m_response.fileLen();
        m_iovCnt = 2;
    }
    // LOG_DEBUG("FileSize: %d, %d totalSize %d", m_response.fileLen(), m_iovCnt, toWriteBytes());
    return true;
}