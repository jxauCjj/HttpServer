#ifndef __SQLCONNRAII__H__
#define __SQLCONNRAII__H__

#include <mysql/mysql.h>
#include "sqlconnpool.h"

// RAII思想 管理从连接池中获得的连接 避免手动归还
class SqlConnRAII{

public:
    SqlConnRAII(MYSQL *&conn, SqlConnPool *pool) {
        assert(pool);
        conn = pool->getConn();
        m_conn = conn;
        m_pool = pool;
    }

    ~SqlConnRAII() {
        if(m_conn) {
            m_pool->freeConn(m_conn);   // 析构时自动归还连接
        }
    }

private:
    MYSQL *m_conn;
    SqlConnPool *m_pool;
};

#endif  //!__SQLCONNRAII__H__