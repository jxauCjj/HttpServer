#ifndef __WEBSERVER__H__
#define __WEBSERVER__H__

#include <cstring>
#include <memory>
#include <unordered_map>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "../logger/log.h"
#include "../http/httpconn.h"
#include "epoller.h"

class WebServer{

public:
    explicit WebServer(int port);   //  初始化
    ~WebServer();

    void start();   // 启动服务器运行

private:
    bool initSocket();

    void handleConn();  // 处理连接事件
    void handleRead(HttpConn &httpConn);  // 处理读数据事件(http请求事件)
    void handleWrite(HttpConn &httpConn); // 处理写数据事件(http响应)
    void closeConn(HttpConn &httpConn);   // 关闭连接

    static void setNonBlocking(int fd);  // 设置描述符为非阻塞读(事件触发必然有数据,无需阻塞读)

private:

    static constexpr int MAX_FD = 65536;    // 最大监听的客户端描述符个数

    int m_port;   // 监听的端口
    int m_listenFd;  // 监听文件描述符
    uint32_t m_listenEvent; // 监听套接字事件
    uint32_t m_connEvent;   // 通信套接字事件

    bool m_isRunning;   // 服务器是否正在运行

    std::unique_ptr<Epoller> m_epoller = nullptr;
    std::unordered_map<int, HttpConn> m_users;  // 存储已创建的用户连接 fd: httpConn
};


#endif  //!__WEBSERVER__H__