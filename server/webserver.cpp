#include "webserver.h"

WebServer::WebServer(int port): 
        m_port(port), m_listenFd(-1), m_isRunning(true), 
        m_epoller(new Epoller())
{
    HttpConn::g_userCount = 0;
    Log::getInstance()->init("./log", ".log", 0);    // 初始化日志对象

    // 初始化监听事件类型
    m_listenEvent = EPOLLRDHUP;
    m_connEvent = EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLONESHOT;  // 理解EPOLLHUP与EPOLLRDHUP的区别

    // 初始化监听套接字(加入到epoll中)
    if(!initSocket()){
        m_isRunning = false;
        LOG_ERROR("Init Socket error !");
    }
}

WebServer::~WebServer(){
    if(m_listenFd > 0){
        close(m_listenFd);  // 关闭连接
    }
}

void WebServer::start() {
    
    if(m_isRunning){
        LOG_INFO("============ Server Start ============");
    }

    while (m_isRunning){
        int eventCnt = m_epoller->wait(-1); // 监听事件 

        for(int i = 0; i < eventCnt; ++i){
            // 获取触发事件的描述符及其对应类型
            int fd = m_epoller->getEventFd(i);
            uint32_t eventsType = m_epoller->getEvents(i);

            if(fd == m_listenFd){
                // 处理新连接
                handleConn();
            }
            else if(eventsType & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)){
                // 关闭连接
                assert(m_users.count(fd) > 0);
                closeConn(m_users[fd]);
            }
            else if(eventsType & EPOLLIN){
                // 读取数据
                assert(m_users.count(fd) > 0);
                handleRead(m_users[fd]);
            }
            else if(eventsType & EPOLLOUT){
                // 写数据
                assert(m_users.count(fd) > 0);
                handleWrite(m_users[fd]);
            }
        }
    }
    
}

bool WebServer::initSocket() {

    int ret = 0;
    struct sockaddr_in addr;
    
    if(m_port < 1024 || m_port > 65535){
        LOG_ERROR("Port: %d error!", m_port);
        return false;
    }

    // 创建TCP监听套接字
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_listenFd < 0){
        LOG_ERROR("Create Listen Socket error! port = %d", m_port);
        return false;
    }

    // 设置端口复用
    int opt = 1;
    ret = setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(int));
    if(ret == -1){
        LOG_ERROR("set socket setsockopt error !");
        return false;
    }

    // 绑定端口
    memset((void *)&addr, 0x00, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_port);

    ret = bind(m_listenFd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0){
        LOG_ERROR("Bind port = %d error!", m_port);
        return false;
    }

    // 监听并加入到epool中
    ret = listen(m_listenFd, 128);
    if(ret < 0){
        LOG_ERROR("Listen error!");
        return false;
    }
    bool res = m_epoller->addFd(m_listenFd, m_listenEvent | EPOLLIN);
    if(!res){
        LOG_ERROR("Epoller add listenfd error!");
        return false;
    }
    setNonBlocking(m_listenFd);
    LOG_INFO("Server port : %d", m_port);
    return true;
}

// 设置文件描述符非阻塞
void WebServer::setNonBlocking(int fd) {
    assert(fd >= 0);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

void WebServer::handleConn(){

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    int fd = accept(m_listenFd, (struct sockaddr *)&addr, &len);

    if(fd < 0){
        LOG_ERROR("Accept Error!");
        return;
    }
    // 加入epoll监听并创建HttpConn(连接)对象
    m_epoller->addFd(fd, m_connEvent | EPOLLIN);
    m_users[fd].init(fd, addr);

    setNonBlocking(fd);

    LOG_INFO("Client %d connect !", fd);
}

void WebServer::closeConn(HttpConn &httpConn) {
    // 删除监听 关闭连接
    LOG_INFO("Client[%d] quit!", httpConn.getFd());
    m_epoller->delFd(httpConn.getFd());
    httpConn.closeConn();
}

void WebServer::handleRead(HttpConn &httpConn) {

    int saveErrno = 0;

    int len = httpConn.read(saveErrno);
    if(len < 0){ 
        if(saveErrno != EAGAIN && saveErrno != EINTR)
            closeConn(httpConn);
        else
            m_epoller->modFd(httpConn.getFd(), m_connEvent | EPOLLIN);        
    }
    else{
        httpConn.process();
        m_epoller->modFd(httpConn.getFd(), m_connEvent | EPOLLOUT);
    }
}

void WebServer::handleWrite(HttpConn &httpConn) {

    int saveErrno = 0;

    int len = httpConn.write(saveErrno);
    if(len < 0){    // 写入失败则分情况讨论
        if(saveErrno != EAGAIN && saveErrno != EINTR)
            closeConn(httpConn);
        else
            m_epoller->modFd(httpConn.getFd(), m_connEvent | EPOLLOUT);        
    }
    else{
        m_epoller->modFd(httpConn.getFd(), m_connEvent | EPOLLIN);
    }
}