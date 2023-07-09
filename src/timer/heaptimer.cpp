#include "timer/heaptimer.h"

void HeapTimer::swapNode(size_t i, size_t j) {
    assert(i >= 0 && i < m_heap.size());
    assert(j >= 0 && j < m_heap.size());
    std::swap(m_heap[i], m_heap[j]);
    m_ref[m_heap[i].id] = i;
    m_ref[m_heap[j].id] = j;
}

bool HeapTimer::shiftDown(size_t index, size_t n) {
    assert(index >= 0 && index < n);

    size_t i = index;
    size_t j = 2 * i + 1;
    while(j < n) {
        if(j + 1 < n && m_heap[j + 1] < m_heap[j]) ++j;
        if(m_heap[i] < m_heap[j]) {
            break;
        }
        swapNode(i, j);
        i = j;
        j = 2 * i + 1;
    }
    return i > index;
}

void HeapTimer::shiftUp(size_t index) {
    if(index == 0)  return;
    assert(index > 0);
    size_t i = index;
    size_t j = (i - 1) / 2;

    while(j > 0) {
        if(m_heap[j] < m_heap[i])   break;
        swapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

// 增加结点
void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);
    if(m_ref.count(id) == 0) {
        // 插入新结点
        size_t index = m_heap.size();
        m_ref[id] = index;
        m_heap.push_back({id, MS(timeout) + Clock::now(), cb});
        shiftUp(index);
    }
    else {
        // 修改已有结点的超时时间
        size_t index = m_ref[id];
        m_heap[index].expires = MS(timeout) + Clock::now();
        if(!shiftDown(index, m_heap.size())) {
            shiftUp(index);
        } 
    }
}

// 删除指定位置的结点
void HeapTimer::del(size_t index) {
    assert(!m_heap.empty() && index >= 0 && index < m_heap.size());
    
    size_t n = m_heap.size() - 1;
    size_t i = index;
    // 交换到堆尾 调整堆
    if(i < n) {
        swapNode(i, n);
        if(!shiftDown(i, n)) {
            shiftUp(i);
        }
    }
    // 删除堆尾元素
    m_ref.erase(m_heap.back().id);
    m_heap.pop_back();
}

// 根据id删除结点 不触发回调
void HeapTimer::doWork(int id) {
    assert(m_ref.count(id) != 0);
    size_t index = m_ref[id];
    del(index);
}

// 调整结点
void HeapTimer::adjust(int id, int timeout) {
    assert(m_ref.count(id));
    
    size_t index = m_ref[id];
    size_t n = m_heap.size();
    m_heap[index].expires = MS(timeout) + Clock::now();
    if(!shiftDown(index, n)) {
        shiftUp(index);
    }
}

// 清空时间堆
void HeapTimer::clear() {
    m_heap.clear();
    m_ref.clear();
}

// 删除超时结点
void HeapTimer::tick() {
    while(!m_heap.empty()) {
        TimerNode &node = m_heap.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0){
            break;
        }
        // 执行回调函数
        node.cb();

        LOG_DEBUG("Timeout Delete Client %d", node.id);
        del(0); // 删除结点
    }
}

// 获取下次检测的间隔
int HeapTimer::getNextTick() {
    tick();
    size_t res = -1;
    if(!m_heap.empty()) {
        res = std::chrono::duration_cast<MS>(m_heap.front().expires - Clock::now()).count();
        if(res < 0) {
            res = 0;
        }
    }
    return res;
}