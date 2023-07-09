#ifndef __HEAPTIMER__H__
#define __HEAPTIMER__H__

#include <chrono>
#include <functional>
#include <vector>
#include <cassert>

#include "logger/log.h"

typedef std::function<void()> TimeoutCallBack;  // 回调函数类型
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;   // 毫秒级别
typedef Clock::time_point TimeStamp;    

/**时间节点*/
struct TimerNode {
    /* data */
    int id;     // 结点唯一表示id(项目中采用文件描述符fd)
    TimeStamp expires;  // 超时时间
    TimeoutCallBack cb; // 回调函数

    bool operator<(const TimerNode &node){  // 重载小于运算符(方便比较结点大小)
        return expires < node.expires;
    }
};

/*时间堆*/
class HeapTimer {

public:
    void add(int id, int timeout, const TimeoutCallBack& cb);   // 增加结点
    void adjust(int id, int timeout);                           // 修改超时时间 调整结点
    void doWork(int id);    // 删除结点 不触发回调(客户端主动关闭连接时使用)
    void clear();           // 清空时间堆
    int getNextTick();      // 获取下次检测时间堆的间隔

private:
    bool shiftDown(size_t index, size_t n); // 下移操作
    void shiftUp(size_t index);     // 上浮操作
    void swapNode(size_t i, size_t j);  // 交换结点
    void del(size_t index); // 删除指定位置的结点
    void tick();    // (定期删除超时结点)

private:
    std::vector<TimerNode> m_heap;
    std::unordered_map<int, size_t> m_ref;      // 记录结点元素在堆中的索引位置
};

#endif  //!__HEAPTIMER__H__