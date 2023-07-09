#ifndef __SQLCONNPOOL__H__
#define __SQLCONNPOOL__H__

#include <mutex>
#include <queue>

#include <semaphore.h>
#include <mysql/mysql.h>
#include "logger/log.h"

// 数据库连接池
class SqlConnPool{

public:
    static SqlConnPool * getInstance(); // 单例模式    
    void init(const char *host, int port, 
            const char *user, const char *passwd, const char *dbName, 
            int connSize = 10);    // 初始化
    
    MYSQL* getConn();               // 获取连接
    void freeConn(MYSQL *conn);     // 归还连接
    void closePool();               // 关闭连接池

private:
    SqlConnPool() = default;
    ~SqlConnPool();

private:
    std::queue<MYSQL *> m_connQue;  // 存储连接的队列
    std::mutex m_mtx;   // 互斥锁
    sem_t m_sem;    // 信号量(线程同步信号量)
};


#endif  //!__SQLCONNPOOL__H__