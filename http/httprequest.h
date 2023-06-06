#ifndef __HTTPREQUEST__H__
#define __HTTPREQUEST__H__

#include <string>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cstring>

#include <mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../logger/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest{

public:
    
    enum LINE_STATE {   // 解析行状态
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    enum PARSE_STATE {  // 解析报文状态
        REQUEST_LINE,   // 请求行
        HEADERS,    // 请求头
        BODY,   // 请求体
        FINISH        
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,     // 解析的结果
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

public:
    HttpRequest() {init();}
    bool parse(Buffer &buff);
    void init();                // 初始化

    const std::string& path() const { return m_path; }
    int statusCode() const { return m_statusCode; }
    bool isKeepAlive() const;

    static void strToUpper(std::string &str);    // 字符串转小写

private:
    LINE_STATE parseLine(Buffer &buff, std::string &line); // 解析出\r\n分隔的每行数据
    bool parseRequestLine(const std::string &line);   // 解析请求行
    bool parseHeader(const std::string &line);        // 解析响应行
    bool parseBody(const std::string &line);          // 解析响应体

    void parsePath(HTTP_CODE status);   // 根据解析状态得到最终的资源路径
    
    void parsePost(const std::string &data);   // 解析表单并数据库验证数据
    void parseFormData(const std::string &data);    // 解析表单数据

    bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);  // 验证表单数据有效性

    HTTP_CODE parseRequest(Buffer &buff); 

private:

    static const std::unordered_set<std::string> DEFAULT_HTML;  // 默认静态页面
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    LINE_STATE m_lineState;
    PARSE_STATE m_parseState;

    std::string m_method, m_path, m_version;  // 请求方法 请求url 协议版本
    std::unordered_map<std::string, std::string> m_headers;   // 存储请求头字段
    std::unordered_map<std::string, std::string> m_post;     // 请求体中的表单数据

    int m_statusCode; // 解析得到的状态码
};

#endif  //!__HTTPREQUEST__H__