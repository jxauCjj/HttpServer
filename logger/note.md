## 日志系统

### 输出信息等级(Level)
- Debug(调试)
- Info(正常输出)
- Warn(警告信息)
- Error(错误)

### 基本功能

- 根据单例模式获取日志对象 Log::getInstance
- 初始化日志对象(日志文件路径path(目录) 等级level 文件信息)
- 根据日期时间、路径文件名、待出输出信息拼接得到日志信息行 并保存在缓冲区中
- 根据是否为异步 将日志信息直接输出到文件 或 阻塞队列中

额外功能：日志的输出行数超过最大行数 或 当前日期与初始输出时的日期不一致(相隔一天) 则根据日期新建日志文件

### 技术细节

- 单例模式
```C++
// 局部静态变量懒汉模式 C++11后保证线程安全,无须加锁
class Single{
    Single(){}
    ~Single(){}

public:
    static Single* getInstance(){
        static Single obj;
        return &obj;
    }
};
``` 
- C++11互斥锁

```C++
std::mutex   // 互斥锁
lock_guard<std::mutex> locker(mutex)    // 构造初始化时自动给mutex加锁，析构销毁时解析(RAII)
```

- 拼接格式串
    - snprintf
    - vsnprintf

- 阻塞队列

1.为何需要先写入阻塞队列

非异步模式下，write会将日志信息直接写入文件中,意味着需要进行I/O操作，占用单个线程中正常的程序执行时间。因此，可以先写入阻塞队列(内存)中，再由某个工作线程从队列中取出元素并写入文件中