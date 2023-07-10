## HttpServer

监听客户端的连接请求，处理请求与响应事件

## 基本功能

1. 监听客户端事件，若为连接事件 加入epoller进行监听
2. 若为读事件,接收数据，解析http请求报文 封装响应报文
3. 若为写事件，将封装好的响应报文发送给客户端

## 程序框架

- epoll I/O多路复用 通知主线程发生的客户端事件
- httpCoon  完成业务逻辑(解析http请求报文并封装响应报文)

## 项目结构
```  
│--CMakeLists.txt     // cmake项目构建文件  
│--README.md     
|--resources         // 网页静态资源
|--bin               // 生成的可执行文件目录
|--log               // 日志文件输出目录
|--src
|  |--main.cpp // 程序入口
|  |--buffer            // 循环缓冲区
|  |  |--buffer.cpp
|  |  └--buffer.h
|  |--http              // 解析并处理http请求和响应报文
|  |  |--httpconn.cpp
|  |  |--httpconn.h
|  |  |--httprequest.cpp
|  |  |--httprequest.h
|  |  |--httpresponse.cpp
|  |  └--httpresponse.h
|  |--logger            // 日志系统
|  |  |--blockqueue.hpp
|  |  |--log.cpp
|  |  └--log.h
|  |--pool              // 线程池和数据库池
|  |  |--sqlconnpool.cpp
|  |  |--sqlconnpool.h
|  |  |--sqlconnRAII.h
|  |  └--threadpool.hpp
|  |--server            // 服务器相关类
|  |  |--epoller.cpp
|  |  |--epoller.h
|  |  |--webserver.cpp
|  |  └--webserver.h
|--└--timer             // 定时器
      |--heaptimer.cpp
      └--heaptimer.h
```
