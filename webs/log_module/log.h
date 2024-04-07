#ifndef _WEBS_LOG_H_
#define _WEBS_LOG_H_

#include <string>
#include <memory>
#include <list>
#include <algorithm>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include "../util/singleton.h"
#include "../util/mutex.h"

namespace webs
{
    class Logger;
    class LoggerManager;
    /* 日志级别 */
    class LogLevel
    {
    public:
        /* 日志级别枚举 */
        enum Level
        {
            /// 未知级别
            UNKNOW = 0,
            /// DEBUG级别
            DEBUG = 1,
            /// INFO级别
            INFO = 2,
            /// WARN级别
            WARN = 3,
            /// ERROR级别
            ERROR = 4,
            /// FATAL级别
            FATAL = 5
        };
        /* 将日志级别转为文本输出 */
        static const char *ToString(LogLevel::Level level);
        /* 将文本转换成日志级别 */
        static LogLevel::Level FromString(const std::string &str);
    };

    /* 日志事件 */
    class LogEvent
    {
    public:
        // 声明ptr类型主要是为了减少代码冗余，不用每次都是用std::shared_ptr<LogEvent>
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                 std::string &thread_name, std::shared_ptr<Logger> logger, LogLevel::Level level);

        /* 返回文件名 */
        const char *getFile() const { return m_file; };
        /* 返回行号 */
        int32_t getLine() const { return m_line; }
        /* 返回耗时 */
        uint32_t getElapse() const { return m_elapse; };
        /* 返回线程ID */
        uint32_t getThreadId() const { return m_threadId; }
        /* 返回协程ID */
        uint32_t getFiberId() const { return m_fiberId; }
        /* 返回时间 */
        uint64_t getTime() const { return m_time; }
        /* 返回线程名称 */
        const std::string &getThreadName() const { return m_threadName; }
        /* 返回日志流的内容 */
        std::string getContent() const { return m_ss.str(); }
        /* 返回日志器 */
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        /* 返回日志等级 */
        LogLevel::Level getLevel() const { return m_level; }
        /* 返回日志内容字符串 */
        std::stringstream &getSS() { return m_ss; }
        /* 格式化写入日志内容 */
        void format(const char *fmt, ...);
        /* 格式化写入日志内容 */
        void format(const char *fmt, va_list al); // va_list也是可变参数的一种

    private:
        /// 文件名
        const char *m_file = nullptr;
        /// 行号
        int32_t m_line = 0;
        /// 程序启动开始到现在的毫秒数
        uint32_t m_elapse = 0;
        /// 线程ID
        uint32_t m_threadId = 0;
        /// 协程ID
        uint32_t m_fiberId = 0;
        /// 时间戳
        uint64_t m_time = 0;
        /// 线程名称
        std::string m_threadName;
        /// 日志内容流
        std::stringstream m_ss;
        /// 日志器
        std::shared_ptr<Logger> m_logger;
        /// 日志等级
        LogLevel::Level m_level;
    };

    /* 日志事件的包装器: 用对象管理资源 */
    class LogEventWrap
    {
    public:
        LogEventWrap(LogEvent::ptr event);
        ~LogEventWrap();
        /* 获取日志事件 */
        LogEvent::ptr getEvent() const { return m_event; };
        /* 获取日志内容流 */
        std::stringstream &getSS();

    private:
        // 封装的日志事件
        LogEvent::ptr m_event;
    };

    /* 日志格式化 */
    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        /**
         * 构造函数，格式化模板
         * %m   消息
         * %p   日志级别
         * %r   累计毫秒数
         * %c   日志名称
         * %t   线程id
         * %n   换行
         * %d   时间
         * %f   文件名
         * %l   行号
         * %T   制表符
         * %F   协程id
         * %N   线程名称
         *
         * 默认格式："%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
         */
        LogFormatter(const std::string &pattern);
        /* 返回格式化日志文本   目前只使用event，其它参数的意义是什么呢 */
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
        std::ostream &format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

        /* 初始化，解析日志模板 */
        void init();
        /* 是否有错误 */
        bool isError() const { return m_error; };

    public:
        /* 日志内容项格式化 为什么使用内部类，好处(推测是封装性)，设计思路是什么；这是一个抽象基类*/
        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            virtual ~FormatItem(){};
            /* 格式化日志到流 纯虚函数 */
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

    private:
        // 日志格式模板
        std::string m_pattern;
        // 日志格式解析后的格式
        std::vector<FormatItem::ptr> m_items;
        // 是否有错误
        bool m_error = false;
    };

    /* 日志输出目标 */
    class LogAppender
    {
        friend class Logger;

    public:
        typedef std::shared_ptr<LogAppender> ptr;
        typedef Spinlock MutexType;

        ~LogAppender(){};
        /* 写入日志 */
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        /* 将日志输出目标的配置转成YAML String */
        virtual std::string toYamlString() = 0;
        /* 获取日志格式器 */
        LogFormatter::ptr getFormatter();
        /* 更改日志格式器 */
        void setFormatter(LogFormatter::ptr val);
        /* 获取日志级别 枚举同样可以作为返回值类型 */
        LogLevel::Level getLevel() const { return m_level; }
        /* 设置日志级别 */
        void setLevel(LogLevel::Level level) { m_level = level; }

    protected:
        // mutex
        MutexType m_mutex;
        // 日志级别
        LogLevel::Level m_level = LogLevel::DEBUG;
        // 是否有自己的日志格式器
        bool m_hasFormatter = false;
        // 日志格式器
        LogFormatter::ptr m_formatter;
    };

    /* Logger类继承自std::enable_shared_from_this<Logger>，这意味着Logger类
    可以安全地生成std::shared_ptr<Logger>实例，以避免在异步情况下可能出现的内存泄漏问题。*/
    /* 日志器 */
    class Logger : public std::enable_shared_from_this<Logger>
    {
        friend class LoggerManager;

    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef Spinlock MutexType;
        /* 日志器的名称 */
        Logger(const std::string &name = "root");
        /* 写日志 */
        void log(LogLevel::Level level, LogEvent::ptr event);
        /* 写debug级别日志 */
        void debug(LogEvent::ptr event);
        /* 写info级别的日志 */
        void info(LogEvent::ptr event);
        /* 写warn级别的日志 */
        void warn(LogEvent::ptr event);
        /* 写error级别的日志 */
        void error(LogEvent::ptr event);
        /* 写fatal级别日志 */
        void fatal(LogEvent::ptr event);
        /* 添加日志目标 */
        void addAppender(LogAppender::ptr appender);
        /* 删除日志目标 */
        void delAppender(LogAppender::ptr appender);
        /* 清空日志目标 */
        void clearAppenders();
        /* 返回日志级别 */
        LogLevel::Level getLevel() const { return m_level; }
        /* 设置日志级别 */
        void setLevel(LogLevel::Level level) { m_level = level; }
        /* 返回日志名称 */
        const std::string &getName() const { return m_name; }
        /* 设置日志格式模板 这两个函数都是设置日志格式器，只不过需要的实参类型不同*/
        void setFormatter(const std::string &val);
        /* 设置日志格式器 */
        void setFormatter(LogFormatter::ptr val);
        /* 获取日志格式器 */
        LogFormatter::ptr getFormatter();
        /* 将日志器的配置转成YAML String */
        std::string toYamlString();

    private:
        /// @brief Mutex
        MutexType m_mutex;
        /// 日志名称
        std::string m_name;
        /// 日志级别
        LogLevel::Level m_level = LogLevel::DEBUG;
        /// 日志目标集合
        std::list<LogAppender::ptr> m_appenders;
        /// 日志格式器
        LogFormatter::ptr m_formatter;
        /// 主日志器
        Logger::ptr m_root;
    };

    /* 输出到控制台 */
    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;
    };

    /* 输出到文件 */
    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename);
        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;

        /* 重新打开日志文件，成功返回true */
        bool reopen();

    private:
        // 文件路径
        std::string m_filename;
        // 文件流 这个是共享资源，需要加锁
        std::ofstream m_filestream;
        // 上次重新打开的时间
        uint64_t m_lasttime = 0;
    };

    /* 日志器的管理类 */
    class LoggerManager
    {
    public:
        typedef Spinlock MutexType;
        LoggerManager();
        /* 获取日志器 */
        Logger::ptr getLogger(const std::string &name);
        /* 初始化 */
        void init();
        /* 返回主日志器 */
        Logger::ptr getRoot() const { return m_root; };
        /* 将所有的日志器配置成YAML */
        std::string toYamlString();

    private:
        // mutex
        MutexType m_mutex;
        // 日志器容器
        std::map<std::string, Logger::ptr> m_loggers;
        // 主日志容器
        Logger::ptr m_root;
    };

    /// 日志器管理类单例模式
    typedef webs::Singleton<LoggerManager> LoggerMgr;

}

/* LogAppender、LoggerManager、Logger中 m_mutex 的初始化是在构造对象时，执行了对应锁的默认构造*/

#endif