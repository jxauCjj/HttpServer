#include "pool/sqlconnpool.h"

SqlConnPool* SqlConnPool::getInstance() {

    static SqlConnPool connPool;
    return &connPool;
}

// 初始化
void SqlConnPool::init(const char *host, int port, 
        const char *user, const char *passwd, const char *dbName, 
        int connSize)
{
    assert(connSize > 0);

    // 根据连接参数创建MySQL连接 并加入到队列中
    for(int i = 0; i < connSize; ++i) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);

        if(!sql) {
            LOG_ERROR("MySql init error");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, passwd, dbName, port, nullptr, 0);
        if(!sql) {
            LOG_ERROR("MySql connect error");
            assert(sql);
        }
        m_connQue.push(sql);
    }
    sem_init(&m_sem, 0, connSize);  //信号量初始化为数据库的连接总数
}    

MYSQL* SqlConnPool::getConn() {

    MYSQL *sql = nullptr;

    // if(m_connQue.empty()) {
    //     LOG_WARN("SqlConnPool Busy");
    //     return nullptr;
    // }
    // 等待信号量
    sem_wait(&m_sem);
    // 互斥访问
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        sql = m_connQue.front();
        m_connQue.pop();
    }
    return sql;
}

void SqlConnPool::freeConn(MYSQL *mysql) {
    if(mysql == nullptr) {
        return;
    }
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        m_connQue.push(mysql);

        LOG_DEBUG("free conn");
    }
    sem_post(&m_sem);    // 唤醒获取连接的线程
}

void SqlConnPool::closePool() {
    // 销毁队列中的所有连接
    std::lock_guard<std::mutex> locker(m_mtx);
    while(!m_connQue.empty()) {
        MYSQL *conn = m_connQue.front();
        m_connQue.pop();
        mysql_close(conn);
    }
    mysql_library_end();

    // 销毁信号量
    sem_destroy(&m_sem);
}

SqlConnPool::~SqlConnPool() {
    closePool();
}