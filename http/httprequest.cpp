#include "httprequest.h"




void HttpRequest::init() {

    m_parseState = REQUEST_LINE;
    m_lineState = LINE_OK;

    m_method.clear();
    m_path.clear();
    m_version.clear();
    m_headers.clear();
}

bool HttpRequest::parse(Buffer &buff) {

    std::string line;

    while((m_lineState = parseLine(buff, line)) == LINE_OK) {
        // LOG_DEBUG("Parse Line %s", line.c_str());
        switch (m_parseState)
        {
        case REQUEST_LINE:
            parseRequestLine(line);
            break;
        case HEADERS:
            parseHeader(line);
            break; 
        case BODY:
            LOG_DEBUG("Parse BODY");
            return true;
            // parseBody(line);
            break;
        default:
            break;
        }
    }
    return true;
}

HttpRequest::LINE_STATE HttpRequest::parseLine(Buffer &buff, std::string &line) {

     // 解析每一行
    std::size_t count = 0;

    for(auto it = buff.peek(); it != buff.beginWrite(); ++it, ++count) {
        if(*it == '\n'){
            if(it != buff.peek() && *(it - 1) == '\r'){
                line.assign(buff.peek(), it - 1);
                buff.hasReadn(count + 1);
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if(*it == '\r'){
            if((it + 1) == buff.beginWrite()){
                return LINE_OPEN;
            }
            else if(*(it + 1) == '\n'){
                line.assign(buff.peek(), it);
                buff.hasReadn(count + 2);

                return LINE_OK;
            }
            return LINE_BAD; // 存在语法错误    
        }
    }
    return LINE_OPEN;
}

bool HttpRequest::parseRequestLine(const std::string &line) {
    // 利用C++11 提供的正则表达式库解析 regex
    std::regex pattern("^([^ ]+) ([^ ]+) (HTTP/[^ ]+)$", std::regex::icase);    // 忽略大小写
    std::smatch subMatch;

    if(std::regex_match(line, subMatch, pattern)) {
        m_method = subMatch[1];
        m_path = subMatch[2];
        m_version = subMatch[3];

        LOG_DEBUG("Method: %s, Path: %s, Version: %s", m_method.c_str(), m_path.c_str(), m_version.c_str());
        m_parseState = HEADERS; // 状态转移
        return true;
    }

    LOG_ERROR("RequestLine: %s error!", line.c_str());
    return false;
}

// 解析响应行
bool HttpRequest::parseHeader(const std::string &line){        
    
    if(line.empty()) {
        m_parseState = BODY;

        for(auto it = m_headers.begin(); it != m_headers.end(); ++it){
            LOG_DEBUG("Request Header %s: %s", it->first.c_str(), it->second.c_str());
        }
        return true;
    }

    std::regex pattern("^([^  :]*): *([^ ]+.*)$");
    std::smatch subMatch;

    if(std::regex_match(line, subMatch, pattern)){
        // LOG_DEBUG("Request Header Line %s", line.c_str());
        m_headers[subMatch[1]] = subMatch[2];
        // LOG_DEBUG("Insert Success");
        return true;
    }
    else {
        LOG_ERROR("Request Header: %s error!", line.c_str());
        return false;
    }
}

// 解析响应体
bool HttpRequest::parseBody(const std::string &line) {
    return true;
}