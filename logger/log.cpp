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

Log::Log(): m_buff(BUFF_SIZE){

    m_path = nullptr;
    m_suffix = nullptr;
    m_fp = nullptr;
    m_lineCount = 0;
    m_isAsync = false;
}

Log::~Log(){
    // 清空阻塞队列
    if(m_writeThread && m_writeThread->joinable()){
        while(!m_queue->empty()){
            m_queue->flush();
        }
        m_queue->close();
        m_writeThread->join();
    }
    
    std::lock_guard<std::mutex> locker(m_mtx);
    if(m_fp){
        flush();
        fclose(m_fp);
    }
}

// 初始化日志对象
void Log::init(const char* path, const char *m_suffix, int queueSize){
    m_path = path;
    m_suffix = m_suffix;

    if(queueSize > 0){
        m_isAsync = true;
        m_queue.reset(new BlockQueue<std::string>(queueSize));  // 创建阻塞队列
        m_writeThread.reset(new std::thread(flushLogThread));   // 创建写线程
    }

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
        ++m_lineCount;  // 行数+1
        int len = snprintf(m_buff.beginWrite(), 128, "%04d-%02d-%02d %02d:%02d:%02d.%06ld ",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, 
            t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        
        m_buff.hasWritten(len);

        // 追加level信息
        len = snprintf(m_buff.beginWrite(), m_buff.writeableBytes(), "%s", LEVEL_INFO[level]);
        m_buff.hasWritten(len);

        va_start(vaList, format);
        len = vsnprintf(m_buff.beginWrite(), m_buff.writeableBytes(), format, vaList); // 输出格式串
        va_end(vaList);
        m_buff.hasWritten(len);

        m_buff.append("\n\0", 2);   // \0表示字符串结尾

        if(m_isAsync && m_queue && !m_queue->full()){
            // TODO
            m_queue->push(m_buff.bufferToString());
        }
        else{
            fputs(m_buff.peek(), m_fp);    // 写入文件
            m_buff.clear();
        }
        flush();    // debug用
    }
}

// 刷新文件缓冲区
void Log::flush(){
    if(m_isAsync)
        m_queue->flush();
    fflush(m_fp);
}

// 从队列中取元素 写入到日志文件中
void Log::asyncWrite(){
    std::string logStr = "";
    while(m_queue->pop(logStr)){
        std::lock_guard<std::mutex> locker(m_mtx);
        fputs(logStr.c_str(), m_fp);
    }

    // std::cout << "Log Writting Finish!!!" << std::endl;
}

void Log::flushLogThread(){
    Log::getInstance()->asyncWrite();
}