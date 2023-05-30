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

    if(len > 0){
        std::string msg = m_rdBuff.bufferToString(); 
        m_wrBuff.append(msg); // Debug 回声服务器

        LOG_INFO("Receive Client [%d] (%s:%d) Message: %s", m_fd, getIP(), getPort(), msg.c_str());
    }

    return len;
}

// 发送数据
ssize_t HttpConn::write(int &saveErrno){
    ssize_t len = -1;
    len = m_wrBuff.writeFd(m_fd, saveErrno);
    
    if(len > 0){
        LOG_INFO("Send Client [%d] (%s:%d) Message", m_fd, getIP(), getPort());
    }
    
    return len;
}

// 解析http请求
bool HttpConn::process() {

    m_request.init();

    m_request.parse(m_rdBuff);  // 解析成功则发送响应数据 失败则说明数据包不完整
    return true;
}