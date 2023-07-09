#ifndef __BLOCKQUEUE__H__
#define __BLOCKQUEUE__H__

#include <cassert>
#include <deque>
#include <mutex>
#include <condition_variable>

// 同步阻塞队列
template<class T>
class BlockQueue{

private:
    std::deque<T> m_que;
    size_t m_capacity;     // 队列容量
    bool m_isClose;
    
    std::mutex m_mtx;   // 互斥锁
    std::condition_variable m_condConsumer; // 消费者条件变量(队列为空则阻塞)
    std::condition_variable m_condProducter;    // 生产者条件变量(队列满则阻塞)

public:
    explicit BlockQueue(int maxCapacity = 1024){
        assert(maxCapacity > 0);
        m_capacity = maxCapacity;
        m_isClose = false;
    }

    void push(const T &item){
        
        std::unique_lock<std::mutex> locker(m_mtx);
        while(m_que.size() >= m_capacity){
            m_condProducter.wait(locker);
        }
        m_que.push_back(item);
        m_condConsumer.notify_one();
    }

    bool pop(T &item){
        std::unique_lock<std::mutex> locker(m_mtx);
        while(m_que.empty()){
            m_condConsumer.wait(locker);
            if(m_isClose)
                return false;
        }
        item = m_que.front();
        m_que.pop_front();
        m_condProducter.notify_one();
        return true;
    }

    bool empty(){
        std::lock_guard<std::mutex> locker(m_mtx);
        return m_que.empty();
    }

    bool full(){
        std::lock_guard<std::mutex> locker(m_mtx);
        return m_que.size() >= m_capacity;
    }

    // 关闭清空队列
    void close(){
        {
            std::lock_guard<std::mutex> locker(m_mtx);
            m_que.clear();
            m_isClose = true;
        }
        m_condConsumer.notify_all();
        m_condProducter.notify_all();
    }

    void flush(){
        m_condConsumer.notify_one();
    }
};

#endif  //!__BLOCKQUEUE__H__