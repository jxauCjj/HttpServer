#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".mp4", "video/mp4"},
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "}
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

/*
函数描述: 初始化HttpResponse对象
函数参数:
    - statusCode: httpRequest对象解析得到的状态码
    - path: 资源文件路径
    - srcDir: 资源文件所在的根目录
    - isKeepAlive: http请求是否为长连接
函数返回值: 无
*/
void HttpResponse::init(int statusCode, const std::string &path, const std::string &srcDir, bool isKeepAlive) {
    m_statusCode = statusCode;
    m_path = path;
    m_srcDir = srcDir;
    m_isKeepAlive = isKeepAlive;

    if(m_mfile){
        unmapFile();
    }
    m_mfile = nullptr;
    m_fileStat = {0};
}

void HttpResponse::unmapFile() {
    if(m_mfile) {
        munmap(m_mfile, m_fileStat.st_size);
        m_mfile = nullptr;
    }
}

void HttpResponse::makeResponse(Buffer &buffer) {

    addStatueLine(buffer);
    addHeaderLine(buffer);
    addContent(buffer);
}

void HttpResponse::addStatueLine(Buffer &buffer) {

    if(CODE_STATUS.count(m_statusCode) == 0) {
        m_statusCode = 400;
        m_path = "/400.html";
    }
    std::string status = CODE_STATUS.find(m_statusCode)->second;
    buffer.append("HTTP/1.1 " + std::to_string(m_statusCode) + " " + status + "\r\n");
}

void HttpResponse::addHeaderLine(Buffer &buffer) {
    buffer.append("Connection: ");
    
    if(m_isKeepAlive) {
        buffer.append("keep-alive\r\n");
        buffer.append("keep-alive: max=6, timeout=120\r\n");
    } else{
        buffer.append("close\r\n");
    }
    buffer.append("Content-Type: " + getFileType() + "\r\n");
}

void HttpResponse::addContent(Buffer &buffer) {

    std::string filePath = m_srcDir + m_path;

    int srcFd = open(filePath.data(), O_RDONLY);
    assert(srcFd >= 0);

    // 内存映射文件    
    // LOG_DEBUG("Response File Path = %s", filePath.data());
    stat(filePath.data(), &m_fileStat);
    m_mfile = (char *)mmap(nullptr, m_fileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);

    assert(m_mfile);
    close(srcFd);
    
    // 增加content-length属性
    buffer.append("Content-Length: " + std::to_string(m_fileStat.st_size) +"\r\n\r\n");
}

std::string HttpResponse::getFileType() {

    std::string::size_type pos = m_path.find_last_of('.');
    if(pos == std::string::npos){
        return "text/plain";
    }
    std::string ext = m_path.substr(pos);
    if(SUFFIX_TYPE.count(ext) == 0){
        return "text/plain";
    }
    return SUFFIX_TYPE.find(ext)->second;
}