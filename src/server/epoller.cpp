#include "server/epoller.h"

Epoller::Epoller(int maxEvents): m_epfd(epoll_create(512)), m_events(maxEvents){
    assert(m_epfd >= 0 && m_events.size() > 0);  
}

Epoller::~Epoller(){
    close(m_epfd);
}

bool Epoller::addFd(int fd, uint32_t eventsType){
    
    if(fd < 0)
        return false;

    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = eventsType;
    return 0 == epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::delFd(int fd){
    if(fd < 0)
        return false;
    return 0 == epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, nullptr);
}

bool Epoller::modFd(int fd, uint32_t eventsType){
    
    if(fd < 0)
        return false;

    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = eventsType;
    return 0 == epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &ev);
}

int Epoller::wait(int timeoutMs){
    return epoll_wait(m_epfd, &m_events[0], static_cast<int>(m_events.size()), timeoutMs);
}

int Epoller::getEventFd(int i) const {
    assert(i >= 0 && i < static_cast<int>(m_events.size()));
    return m_events[i].data.fd;
}

uint32_t Epoller::getEvents(int i) const {
    assert(i >= 0 && i < static_cast<int>(m_events.size()));
    return m_events[i].events;
}