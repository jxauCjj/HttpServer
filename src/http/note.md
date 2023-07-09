## HttpReques类

负责解析 接收到的请求数据并存储解析的结果

解析步骤

根据\r\n符 解析出每一行 查看行数据是否完整

 enum LINE_STATUS
    {
        LINE_OK = 0,    // 行完整
        LINE_BAD,       // 行语法错误
        LINE_OPEN       // 不完整
    };

    enum PARSE_STATE {  // 解析状态
        REQUEST_LINE,   // 请求行
        HEADERS,    // 请求头
        BODY,   // 请求体
        FINISH, //        
    };

请求行 请求方法 url(path) 协议版本 
请求头 -> 
请求体

HttpRequest对象 负责解析请求 
                在此类中完成对资源文件的校
                根据请求得到资源文件路径 和 对应的状态码(200 404 500)

HttpResponse对象只负责根据解析的结果组装响应报文，不再进行资源的检验

状态行 
响应行 Connection Content-type Content-length(由文件大小决定)
响应体