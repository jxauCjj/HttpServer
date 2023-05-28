#ifndef __HTTPCONN__H__
#define __HTTPCONN__H__

#include <cstring>
#include <cassert>
#include <atomic>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../buffer/buffer.h"
#include "../logger/log.h"
#include "httprequest.h"

/**
 * http连接对象类
 * 处理读写数据 解析请求 组装响应报文逻辑
*/
class HttpConn{

public:
    static std::atomic<int> g_userCount; // 已连接的客户端个数(原子类型 保证多线程访问安全)
    static const char *g_srcDir;    // 资源文件所在目录

public:
    HttpConn();

    void init(int fd, struct sockaddr_in &addr);   // 初始化
    void closeConn();   // 关闭连接

    int getFd() const;  // 获取描述符
    const char* getIP() const;  // 获取IP地址
    int getPort() const;    // 获取端口

    ssize_t read(int &saveErrno);   // 读请求数据到rdBuff
    ssize_t write(int &saveErrno);  // 发送数据

    bool process(); // 解析报文  

private:
    int m_fd;   // socket描述符
    struct sockaddr_in m_addr;    // 客户端地址 端口信息
    bool m_isClose; // 表示是否已关闭

    // 读写缓冲区
    Buffer m_rdBuff;    // 临时存放接收到的响应报文数据
    Buffer m_wrBuff;    // 存放响应报文头部

    HttpRequest m_request;  // 解析请求报文对象

};


#endif  //!__HTTPCONN__H__