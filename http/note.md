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