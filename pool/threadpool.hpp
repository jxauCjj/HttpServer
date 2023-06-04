#ifndef __THREADPOOL__H__
#define __THREADPOOL__H__

#include <queue>
#include <thread>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <cassert>

class ThreadPool{

public:

    explicit ThreadPool(int threadCount = 8) {
        assert(threadCount > 0);
        // 开启工作线程
        for(int i = 0; i < threadCount; ++i) {
            std::thread(std::bind(&ThreadPool::worker, this)).detach();  // 设置线程分离
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> locker(m_tex);
            m_isClose = true;
        }
        m_cond.notify_all();
    }

    template<class F>
    void addTask(F &&task) {    // 添加任务(模板函数)
        
        {
            std::lock_guard<std::mutex> locker(m_tex);
            m_tasks.emplace(std::forward<F>(task));
        }
        m_cond.notify_one();
    }

    // 工作线程
    void worker() {

        std::unique_lock<std::mutex> locker(m_tex);
        
        while(true){
            while(m_tasks.empty()) {
                m_cond.wait(locker);
            }
            if(m_isClose) {
                locker.unlock();
                break;
            }
            else {
                auto task = std::move(m_tasks.front());
                m_tasks.pop();
                locker.unlock();
                task(); // 执行任务
                locker.lock();
            }
        }    
    }

private:
    std::mutex m_tex;   // 互斥锁
    std::condition_variable m_cond; // 唤醒工作线程的条件变量
    std::queue<std::function<void()>> m_tasks; // 任务队列 (仿函数对象)
    bool m_isClose = false; //线程池是否关闭
};

#endif  //!__THREADPOOL__H__