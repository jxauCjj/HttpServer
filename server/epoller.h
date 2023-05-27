#ifndef __EPOLLER__H__
#define __EPOLLER__H__

#include <vector>
#include <cassert>

#include <sys/epoll.h>
#include <unistd.h>

/* epoll类的封装*/
class Epoller{

public:
    explicit Epoller(int maxEvents = 1024);
    ~Epoller();

    // 添加 删除 修改对应的监听fd
    bool addFd(int fd, uint32_t eventsType);
    bool delFd(int fd);
    bool modFd(int fd, uint32_t eventsType);

    int wait(int timeoutMS);  // 监听套接字事件
    int getEventFd(int i) const;  // 获取监听到的第i个事件对应的描述符
    uint32_t getEvents(int i) const;    // 获取监听到的第i个事件的类型  


private:
    int m_epfd;     // 创建出的epoll描述符
    std::vector<struct epoll_event> m_events;   // 监听描述符事件数组
};

#endif  //!__EPOLLER__H__