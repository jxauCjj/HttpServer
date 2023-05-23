#include "log.h"

const char *Log::LEVEL_INFO[4] = {
    "[debug]: ",
    "[info]: ",
    "[warn]: ",
    "[error]: "
};

// 局部静态变量懒汉模式
Log* Log::getInstance(){
    static Log logger;
    return &logger;
}

Log::Log(){
    m_path = nullptr;
    m_suffix = nullptr;
    m_fp = nullptr;
    m_lineCount = 0;
    m_buffPos = 0;
    m_isAsync = false;
}

Log::~Log(){
    if(m_fp){
        fclose(m_fp);
    }
}

// 初始化日志对象
void Log::init(const char* path, const char *m_suffix){
    m_path = path;
    m_suffix = m_suffix;

    // 根据当前日期得到日志文件名
    char fileName[LOG_FILE_LEN] = {0};
    time_t timer = time(nullptr);
    struct tm t = *localtime(&timer);

    snprintf(fileName, LOG_FILE_LEN - 1, "%s/%04d_%02d_%02d%s", 
                m_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, m_suffix);
    m_mday = t.tm_mday; // 记录月份第几天
    
    // 打开日志文件(m_fp共享变量需要互斥访问)
    {
        std::lock_guard<std::mutex> locker(m_mtx);  //(出作用域(析构)时自动解锁)

        if(m_fp != nullptr){
            fclose(m_fp);
        }

        m_fp = fopen(fileName, "a");
        if(m_fp == nullptr){
            // 目录不存在则新建
            mkdir(m_path, 0777);
            m_fp = fopen(fileName, "a");
        }
    }
    assert(m_fp != nullptr);
}

// 写入日志文件
void Log::write(int level, const char *format, ...){

    // 根据当前日期和输入拼接格式串
    
    time_t timer = time(NULL);
    struct tm t = *localtime(&timer);
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    va_list vaList;

    // 判断日期或日志行数是否有效
    if(m_mday != t.tm_mday || (m_lineCount > 0 && m_lineCount % MAX_LINES == 0)){
        
        std::unique_lock<std::mutex> locker(m_mtx);
        locker.unlock();
        
        char fileName[LOG_FILE_LEN] = {0};
        char tail[36] = {0};

        snprintf(tail, 35, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        if(m_mday != t.tm_mday){
            m_mday = t.tm_mday;
            m_lineCount = 0;
            snprintf(fileName, LOG_FILE_LEN - 1, "%s/%s%s", m_path, tail, m_suffix);
        }
        else{
            snprintf(fileName, LOG_FILE_LEN - 1, "%s/%s_%d%s", m_path, tail, m_lineCount / MAX_LINES, m_suffix);
        }

        locker.lock();
        flush();
        fclose(m_fp);
        m_fp = fopen(fileName, "a");    // 重新打开新的文件
        assert(m_fp != nullptr);
    }

    // 根据当前日期和输入拼接格式串
    {
        std::unique_lock<std::mutex> locker(m_mtx);
        ++m_lineCount;
        int len = snprintf(m_buff + m_buffPos, 128, "%04d-%02d-%02d %02d:%02d:%02d.%06ld ",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, 
            t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        
        m_buffPos += len;

        // 追加level信息
        len = snprintf(m_buff + m_buffPos, BUFF_SIZE - m_buffPos - 2, "%s", LEVEL_INFO[level]);
        m_buffPos += len;

        va_start(vaList, format);
        len = vsnprintf(m_buff + m_buffPos, BUFF_SIZE - m_buffPos - 2, format, vaList); // 输出格式串
        va_end(vaList);

        m_buffPos += len;
        m_buff[m_buffPos++] = '\n';
        m_buff[m_buffPos++] = '\0';

        if(m_isAsync){
            // TODO
        }
        else{
            fputs(m_buff, m_fp);    // 写入文件
        }
        flush();    // debug用
        m_buffPos = 0;
    }
}

// 刷新文件缓冲区
void Log::flush(){
    fflush(m_fp);
}