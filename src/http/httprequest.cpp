#include "http/httprequest.h"


const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture"};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/register", 0}, {"/login", 1},  };  // 登录为0 注册为1

static const char *srcDir = "./resources";

void HttpRequest::strToUpper(std::string &str) {
    std::transform(str.begin(), str.end(), str.begin(), toupper);
}

void HttpRequest::init() {

    m_parseState = REQUEST_LINE;
    m_lineState = LINE_OK;
    m_statusCode = -1;

    m_method.clear();
    m_path.clear();
    m_version.clear();
    m_headers.clear();
    m_post.clear();
}

bool HttpRequest::parse(Buffer &buff) {
    
    HTTP_CODE status = parseRequest(buff);
    
    // LOG_DEBUG("Parse Request Finish -------");
    if(status == NO_REQUEST){
        m_statusCode = -1;
        return false;           // 请求报文不完整
    }
    parsePath(status);
    
    // LOG_DEBUG("Parse Finish:  Method = %s, Path = %s, Version = %s, StatusCode = %d",
    //     m_method.c_str(), m_path.c_str(), m_version.c_str(), m_statusCode);

    return true;
}

void HttpRequest::parsePath(HTTP_CODE status) {

    // LOG_DEBUG("Start Parse Path-----");

    if(status == BAD_REQUEST) {
        m_path = "/400.html";
        m_statusCode = 400;
    }
    else {
        if(m_path == "/") {
            m_path = "/index.html";
            m_statusCode = 200;
        }
        else if(DEFAULT_HTML.count(m_path)) {
            m_path += ".html";
            m_statusCode = 200;
        }
        else {
            // 其他情况查看资源是否存在和访问权限
            struct stat mmFileStat;
            std::string filePath = srcDir + m_path;

            if(stat(filePath.c_str(), &mmFileStat) < 0 || S_ISDIR(mmFileStat.st_mode)) {
                m_path = "/404.html";   // Not Found 资源不存在
                m_statusCode = 404;
            }
            else if(!(mmFileStat.st_mode & S_IROTH)){
                m_path = "/403.html";
                m_statusCode = 403; // forbidden 无权限访问
            }
            else{
                m_statusCode = 200;
            }
        }
    }
}

HttpRequest::HTTP_CODE HttpRequest::parseRequest(Buffer &buff) {

    std::string line;

    while(m_parseState != FINISH && ((m_parseState == BODY && m_lineState == LINE_OK) || ((m_lineState = parseLine(buff, line)) == LINE_OK))) {
        // LOG_DEBUG("Parse Line %s", line.c_str());
        switch (m_parseState)
        {
        case REQUEST_LINE:
            if(!parseRequestLine(line)){ 
                return BAD_REQUEST; // BAD_REQUEST表示请求报文语法错误
            }
            break;
        case HEADERS:
            if(!parseHeader(line)) {
                return BAD_REQUEST;
            }
            break; 
        case BODY:
            LOG_DEBUG("Parse BODY");
            line = buff.bufferToString();
            buff.hasReadn(line.size());
            parseBody(line);
            break;
        default:
            break;
        }
    }
    if(m_parseState == FINISH){
        return GET_REQUEST; // 请求报文完整
    }
    else {
        return m_lineState == LINE_BAD ? BAD_REQUEST : NO_REQUEST;  // NO_REQUEST表示请求报文不完整
    }
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

        strToUpper(m_method);
        // strToUpper(m_path); 
        strToUpper(m_version);

        LOG_DEBUG("Method: %s, Path: %s, Version: %s", m_method.c_str(), m_path.c_str(), m_version.c_str());
        m_parseState = HEADERS; // 状态转移
        return true;
    }

    LOG_ERROR("RequestLine: %s error!", line.c_str());
    return false;
}

// 解析请求头字段
bool HttpRequest::parseHeader(const std::string &line){        
    
    if(line.empty()) {
        m_parseState = (m_method == "POST" ? BODY : FINISH);      // post请求才解析请求头
        // for(auto it = m_headers.begin(); it != m_headers.end(); ++it){
        //     LOG_DEBUG("Request Header %s: %s", it->first.c_str(), it->second.c_str());
        // }
        return true;
    }

    std::regex pattern("^([^  :]*): *([^ ]+.*)$");
    std::smatch subMatch;

    if(std::regex_match(line, subMatch, pattern)){
        // LOG_DEBUG("Request Header Line %s", line.c_str());
        std::string key = subMatch[1];
        std::string value = subMatch[2];

        strToUpper(key);    // 全部转为大写(避免判断大小写)
        
        m_headers[key] = value;
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
    
    LOG_DEBUG("Body %s, len: %d", line.c_str(), line.size());

    // 解析表单数据并验证数据
    parsePost(line);
    m_parseState = FINISH;
    return true;
}

bool HttpRequest::isKeepAlive() const {
    if(m_headers.count("CONNECTION")) {
        return m_headers.find("CONNECTION")->second == "keep-alive";
    }
    else {
        return false;
    }
}

void HttpRequest::parsePost(const std::string &data) {
    
    if(DEFAULT_HTML_TAG.count(m_path)) {
        parseFormData(data);
        int tag = DEFAULT_HTML_TAG.find(m_path)->second;    // 根据tag判断是否为登录或注册
        LOG_DEBUG("tag = %d", tag);

        bool isLogin = (tag == 1);
        if(UserVerify(m_post["username"], m_post["password"], isLogin)){
            m_path = "/welcome.html";
        }
        else {
            m_path = "/error.html";
        }
    }
}

void HttpRequest::parseFormData(const std::string &data) {
    
    // 解析数据
    std::size_t i = 0, j = 0;
    std::string key, value;
    for(; j < data.size(); ++j) {
        
        switch (data[j])
        {
        case '=':
            key = data.substr(i, j - i);
            i = j + 1;
            break;
        case '&':
            value = data.substr(i, j - i);
            i = j + 1;
            m_post.insert({key, value});
            LOG_DEBUG("Form data %s = %s", key.c_str(), value.c_str());
        default:
            break;
        }
    }

    assert(i <= j);
    if(i < j){
        value = data.substr(i, j - i);
        m_post.insert({key, value});

        LOG_DEBUG("Form data %s = %s", key.c_str(), value.c_str());
    }
}

// 验证表单数据的有效性
bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool isLogin) {
    if(name == "" || pwd == "") { return false; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql = nullptr;
    
    SqlConnRAII connRAII(sql,  SqlConnPool::getInstance()); // 析构时自动归还连接
    assert(sql);
    
    bool flag = false;
    unsigned int j = 0;
    char order[256] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    
    if(!isLogin) { flag = true; }
    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, passwd FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res);
        return false; 
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        /* 注册行为 且 用户名未被使用*/
        if(isLogin) {
            if(pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        } 
        else { 
            flag = false; 
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    /* 注册行为 且 用户名未被使用*/
    if(!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, passwd) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    // SqlConnPool::Instance()->FreeConn(sql); //又归还？？
    LOG_DEBUG( "UserVerify success!!");
    return flag;
}