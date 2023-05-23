#ifndef LOG_H
#define LOG_H

#include <cstdio>
#include <ctime>
#include <cassert>
#include <cstdarg>
#include <mutex>
#include <sys/stat.h>
#include <sys/time.h>


class Log{

private:
    // 一些常量
    static constexpr int LOG_FILE_LEN = 256;    // 日志文件名长度
    static constexpr int MAX_LINES = 50000;     // 单个日志文件最大行数
    static constexpr int BUFF_SIZE = 2048;

    static const char *LEVEL_INFO[4];   // level日志串信息

private:
    const char* m_path; // 日志文件所在目录
    const char* m_suffix;   // 日志文件后缀
    FILE *m_fp;         // 日志文件指针
    int m_lineCount;    // 日志行数
    int m_mday;         // 当前日志天数

    bool m_isAsync;     // 是否为异步模式

    std::mutex m_mtx;   // 互斥量

    // 日志行缓冲区
    char m_buff[BUFF_SIZE]; 
    int m_buffPos;  // 读取的位置

private:
    Log();
    virtual ~Log();

public:
    static Log *getInstance();  // 单例模式
    void init(const char *path, const char *suffix);    // 初始化日志

    void write(int level, const char *format, ...); // 输出格式串到日志文件中
    void flush();
};

#define LOG_DEBUG(format, ...) { Log::getInstance()->write(0, format, ##__VA_ARGS__); }
#define LOG_INFO(format, ...) { Log::getInstance()->write(1, format, ##__VA_ARGS__); }
#define LOG_WARN(format, ...) { Log::getInstance()->write(2, format, ##__VA_ARGS__); }
#define LOG_ERROR(format, ...) { Log::getInstance()->write(3, format, ##__VA_ARGS__); }

#endif