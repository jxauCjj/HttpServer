#include "server/webserver.h"

WebServer::WebServer(int port): 
        m_port(port), m_listenFd(-1), m_timoutMS(30000), m_isRunning(true),
        m_epoller(new Epoller()), m_timer(new HeapTimer()), m_threadPool(new ThreadPool())
{
    HttpConn::g_userCount = 0;
    Log::getInstance()->init("./log", ".log", 0);    // 初始化日志对象

    // 初始化数据库连接池 TODO 参数暂时写死
    SqlConnPool::getInstance()->init("localhost", 3306, "root", "root", "yourdb", 10); /* Mysql配置 */

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
    int ms = -1;
    while (m_isRunning){
        
        if(m_timoutMS > 0)
            ms = m_timer->getNextTick();

        int eventCnt = m_epoller->wait(ms); // 监听事件 
        
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
                if(m_timoutMS > 0)
                    m_timer->doWork(fd);    // 删除时间堆上的结点(不调用回调函数)
                closeConn(m_users[fd]);
            }
            else if(eventsType & EPOLLIN){
                // 读取数据
                assert(m_users.count(fd) > 0);
                if(m_timoutMS > 0)  extentTimer(m_users[fd]);
                m_threadPool->addTask(std::bind(&WebServer::handleRead, this, std::ref(m_users[fd])));
                // handleRead(m_users[fd]);
            }
            else if(eventsType & EPOLLOUT){
                // 写数据
                assert(m_users.count(fd) > 0);
                if(m_timoutMS > 0)  extentTimer(m_users[fd]);
                m_threadPool->addTask(std::bind(&WebServer::handleWrite, this, std::ref(m_users[fd]))); // bind 默认值传递
                
                // handleWrite(m_users[fd]);
            }
            else {
                LOG_ERROR("Unexcepted Event");
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
    else if(HttpConn::g_userCount >= MAX_FD) {  // 连接数达到上限值
        LOG_WARN("Client is full!");
        close(fd);
        return;
    }
    // 加入epoll监听并创建HttpConn(连接)对象
    m_epoller->addFd(fd, m_connEvent | EPOLLIN);
    m_users[fd].init(fd, addr);
    
    if(m_timoutMS > 0)
        m_timer->add(fd, m_timoutMS, std::bind(&WebServer::closeConn, this, std::ref(m_users[fd])));    // 添加连接结点到时间堆(小根堆)

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
    else{// 读取成功则直接处理
        if(httpConn.process())
            m_epoller->modFd(httpConn.getFd(), m_connEvent | EPOLLOUT);
        else
            m_epoller->modFd(httpConn.getFd(), m_connEvent | EPOLLIN);
    }
}

void WebServer::handleWrite(HttpConn &httpConn) {

    int saveErrno = 0;

    int len = httpConn.write(saveErrno);
    if(len < 0){    // 写入失败则分情况讨论
        if(saveErrno != EAGAIN && saveErrno != EINTR)
            closeConn(httpConn);
        else{
            m_epoller->modFd(httpConn.getFd(), m_connEvent | EPOLLOUT);
            // LOG_DEBUG("Write Again");
        }        
    }
    else{
        if(httpConn.isKeepAlive()){
            m_epoller->modFd(httpConn.getFd(), m_connEvent | EPOLLIN);
        }
        else {
            closeConn(httpConn); // 短连接则直接关闭
        }
        // closeConn(httpConn);
        // LOG_DEBUG("Write Complete");
    }
}

void WebServer::extentTimer(HttpConn &httpConn) {
    m_timer->adjust(httpConn.getFd(), m_timoutMS);
}