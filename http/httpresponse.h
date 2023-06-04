#ifndef __HTTPRESPONSE__H__
#define __HTTPRESPONSE__H__

#include <string>
#include <unordered_map>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "../buffer/buffer.h"
#include "../logger/log.h"

class HttpResponse{

public:
    void init(int statusCode, const std::string &path, const std::string &srcDir, bool isKeepAlive);

    std::size_t fileLen() const { return m_mfile ? m_fileStat.st_size : 0; }
    char* fileAddr() const { return m_mfile; }

    void makeResponse(Buffer &buff);    // 组装响应报文
    void unmapFile();  // 解除文件映射关系

private:
    
    void addStatueLine(Buffer &buff);   // 组装状态行
    void addHeaderLine(Buffer &buff);   // 组装响应行
    void addContent(Buffer &buff);  // 组装响应体(映射文件)

    std::string getFileType();  // 获取文件的MINIE类型

private:

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;

    std::string m_srcDir;   // 资源文件根路径
    std::string m_path;     // 资源文件名
    int m_statusCode = -1;   // 状态码
    bool m_isKeepAlive = false; // 是否为持久连接

    char *m_mfile = nullptr;  // 资源文件内存映射地址
    struct stat m_fileStat = {0};    // 资源文件属性信息
};

#endif  //!__HTTPRESPONSE__H__