#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <functional>
#include <map>
#include <tuple>
#include "../util/util.h"
#include "../config/config.h"
#include <yaml-cpp/yaml.h>

namespace webs
{
    /* 将日志级别转为文本输出 */
    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        break;

            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
#undef XX
        default:
            return "UNKNON";
        }
        return "UNKNON";
    }
    /* 将文本转换成日志级别 */
    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
#define XX(level, v)            \
    if (str == #v)              \
    {                           \
        return LogLevel::level; \
    }
        XX(DEBUG, debug);
        XX(INFO, info);
        XX(WARN, warn);
        XX(ERROR, error);
        XX(FATAL, fatal);

        XX(DEBUG, DEBUG);
        XX(INFO, INFO);
        XX(WARN, WARN);
        XX(ERROR, ERROR);
        XX(FATAL, FATAL);
        return LogLevel::UNKNOW;
#undef XX
    }

    /* LogEvent的构造函数 */
    LogEvent::LogEvent(const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                       const std::string &thread_name, std::shared_ptr<Logger> logger, LogLevel::Level level)
        : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time), m_threadName(thread_name), m_logger(logger), m_level(level)
    {
    }
    /* 格式化写入日志内容 */
    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }
    /* 格式化写入日志内容 */
    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (len != -1)
        {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    /* LogEventWrap 的构造函数 */
    LogEventWrap::LogEventWrap(LogEvent::ptr event) : m_event(event)
    {
    }
    /* LogEventWrap 的析构函数 */
    LogEventWrap::~LogEventWrap()
    {
        // 输出日志
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }
    /* 获取日志内容流 */
    std::stringstream &LogEventWrap::getSS()
    {
        return m_event->getSS();
    }

    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str = "") {}
        /* 将日志内容输出到输出流中 */
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str = "") {}
        /* 将日志等级输出到输出流中 */
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << LogLevel::ToString(level);
        }
    };

    class ElaspseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElaspseFormatItem(const std::string &str = "") {}
        /* 将耗时输出到输出流中 */
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getTime();
        }
    };

    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            /* 将日志名称输出到输出流中 */
            os << event->getLogger()->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            /* 将线程id输出到输出流中 */
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            /* 将线程id输出到输出流中 */
            os << event->getFiberId();
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadNameFormatItem(const std::string &str) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            /* 将线程名称输出到输出流中 */
            os << event->getThreadName();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S") : m_format(format)
        {
            if (m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }
        /* 将时间以指定格式输出到输出流中 */
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_format;
    };

    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FilenameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            /* 将文件名输出到输出流中 */
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            /* 将行号输出到输出流中 */
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            /* 将 空行 输出到输出流中 */
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &str) : m_string(str) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            /* 将 string 输出到输出流中 */
            os << m_string;
        }

    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            /* 将 string 输出到输出流中 */
            os << "\t";
        }
    };

    LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
    {
        init();
    }

    /* 返回格式化日志文本；依据item的类型输出对应的格式化信息 */
    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        std::stringstream ss;
        // 依据 m_item 的类型调用对应类型的 format函数
        for (auto &i : m_items)
        {
            // 将内容输出到 ss 中
            i->format(ss, logger, level, event);
        }
        return ss.str();
    }

    std::ostream &LogFormatter::format(std::ostream &ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        for (auto &i : m_items)
        {
            i->format(ofs, logger, level, event);
        }
        return ofs;
    }

    /* 初始化，解析日志模板 %xxx{%xxx%xx}*/
    void LogFormatter::init()
    {
        // std format type
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (int i = 0; i < m_pattern.size(); ++i)
        {
            if (m_pattern[i] != '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }
            if (i + 1 < m_pattern.size())
            {
                if (m_pattern[i + 1] == '%')
                {
                    nstr.append(1, '%');
                    continue;
                }
            }

            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;
            std::string str;
            std::string fmt;

            // 匹配到 %
            while (n < m_pattern.size())
            {
                // fmt_status = 0, m_pattern[n]不是字母且不是'{'、'}'
                if (!fmt_status && !isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}')
                {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }
                // 解析 { } 中的格式
                if (fmt_status == 0)
                {
                    if (m_pattern[n] == '{')
                    {
                        // 读取 % 至 { 之间的字符
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_begin = n;
                        fmt_status = 1; // 解析格式
                        ++n;
                        continue;
                    }
                }
                else if (fmt_status == 1)
                {
                    if (m_pattern[n] == '}')
                    {
                        // 读取 { 至 } 之间的字符
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }
                ++n;
                if (n == m_pattern.size())
                {
                    str = m_pattern.substr(i + 1);
                }
            }

            if (fmt_status == 0)
            {
                if (!nstr.empty())
                {
                    // nstr 是普通字符串，不包含 % ，之后会被格式化为字符串类型; make_tuple使用了完美转发
                    vec.push_back(std::make_tuple(nstr, std::string(), 1));
                    nstr.clear();
                }
                vec.push_back(std::make_tuple(str, fmt, 0));
                i = n - 1;
            }
            else if (fmt_status == 1)
            {
                // [整个模式字符串] - [从错误位置开始的子字符串]  substr(i):从i开始一直到结尾
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                m_error = true;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
            }
        }
        if (!nstr.empty())
        {
            // nstr 是纯字符串
            vec.push_back(std::make_tuple(nstr, std::string(), 1));
        }

        // 创建键值对： 格式-生成ptr对象的lamba表达式；注意是静态局部变量，避免多次初始化
        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
        // 注意这里是基类智能指针，管理派生类对象
#define XX(str, C)                                                               \
    {                                                                            \
        #str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }
            XX(m, MessageFormatItem),    // m: 消息
            XX(p, LevelFormatItem),      // p: 日志级别
            XX(r, ElaspseFormatItem),    // r: 累计毫秒数
            XX(c, NameFormatItem),       // c: 日志名称
            XX(t, ThreadIdFormatItem),   // t: 线程id
            XX(n, NewLineFormatItem),    // n: 换行
            XX(d, DateTimeFormatItem),   // d: 时间
            XX(f, FilenameFormatItem),   // f: 文件名
            XX(l, LineFormatItem),       // l: 行号
            XX(T, TabFormatItem),        // T: Tab
            XX(F, FiberIdFormatItem),    // F: 协程id
            XX(N, ThreadNameFormatItem), // N: 线程名称
#undef XX
        };

        for (auto &i : vec)
        {
            if (std::get<2>(i) == 0)
            {
                // std::get<0>(i)是纯字符串
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end())
                {
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                    m_error = true;
                }
                else
                {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
        }
    }

    /* 获取日志格式器 */
    LogFormatter::ptr LogAppender::getFormatter()
    {
        // 这里使用了 MutexType的拷贝构造 和 对应锁的移动构造。同时使用局部对象管理锁资源，确保可以自动释放锁
        MutexType::Lock lock(m_mutex);
        return m_formatter;
    }

    /* 更改日志格式器 */
    void LogAppender::setFormatter(LogFormatter::ptr val)
    {
        MutexType::Lock lock(m_mutex);
        m_formatter = val;
        // 可能是为了防止传入一个空对象
        if (m_formatter)
        {
            m_hasFormatter = true;
        }
        else
        {
            m_hasFormatter = false;
        }
    }

    Logger::Logger(const std::string &name) : m_name(name)
    {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    }

    /* 重新打开文件 */
    bool FileLogAppender::reopen()
    {
        MutexType::Lock lock(m_mutex);
        // 需要先关闭文件流，确保重新打开时，不会因其他进程使用而无法访问
        if (m_filestream)
        {
            m_filestream.close();
        }
        return FSUtil::OpenForWrite(m_filestream, m_filename, std::ios::app);
    }

    /* 有两种不同级别的日志，文件、终端输出 */
    void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        // 打开文件流，绑定文件
        if (level >= m_level)
        {
            uint64_t now_time = event->getTime();
            if (now_time > m_lasttime + 3)
            {
                reopen();
                m_lasttime = now_time;
            }
            // m_pattern的format函数
            MutexType::Lock lock(m_mutex);
            if (!m_formatter->format(m_filestream, logger, level, event))
            {
                std::cout << "error" << std::endl;
            }
        }
    }

    /* 终端输出 */
    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            MutexType::Lock lock(m_mutex);
            m_formatter->format(std::cout, logger, level, event);
        }
    }

    /* 写日志 */
    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        // 优先输出list中每一个元素的log，如果为空，输出主日志器
        if (level >= m_level)
        {
            Logger::ptr self = shared_from_this(); // 返回一个当前类的std::share_ptr
            if (!m_appenders.empty())
            {
                for (auto i : m_appenders)
                {
                    i->log(self, level, event);
                }
            }
            else if (m_root)
            {
                // 主日志器默认输出到终端
                m_root->log(level, event);
            }
        }
    }

    /* 写debug级别日志 */
    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::DEBUG, event);
    }

    /* 写info级别的日志 */
    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::INFO, event);
    }

    /* 写warn级别的日志 */
    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::WARN, event);
    }

    /* 写error级别的日志 */
    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::ERROR, event);
    }

    /* 写fatal级别日志 */
    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::FATAL, event);
    }

    /* 添加日志目标 */
    void Logger::addAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        // 如果没有设置格式器
        if (!appender->getFormatter())
        {
            /* 添加日志器的时候并没有设置m_hasformat为true（由自己的set函数设置） */
            // appender->setFormatter(m_formatter);
            MutexType::Lock lock(appender->m_mutex);
            appender->m_formatter = m_formatter;
        }
        m_appenders.push_back(appender);
    }

    /* 删除日志目标；仅删除第一个相等的 */
    void Logger::delAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it){
            if (*it == appender)// 可以直接比较两个智能指针
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    /* 清空日志目标 */
    void Logger::clearAppenders()
    {
        MutexType::Lock lock(m_mutex);
        m_appenders.clear();
    }

    /* 设置logger的日志器，如果m_appenders没有设置自己的专属日志器格式，则进行同步更新格式 */
    void Logger::setFormatter(LogFormatter::ptr val)
    {
        MutexType::Lock lock(m_mutex);
        m_formatter = val;
        for (auto &i : m_appenders)
        {
            MutexType::Lock ll(i->m_mutex);
            if (!i->m_hasFormatter)
            {
                i->m_formatter = m_formatter;
            }
        }
    }

    /* 设置日志格式器 */
    void Logger::setFormatter(const std::string &val)
    {
        std::cout << "-------" << std::endl;
        MutexType::Lock lock(m_mutex);
        LogFormatter::ptr new_val(new LogFormatter(val));
        if (new_val->isError())
        {
            std::cout << "Logger setFormatter name = " << m_name
                      << " value = " << val << " invalid formatter" << std::endl;
            return;
        }
        setFormatter(new_val);
        // m_formatter.reset(LogFormatter::ptr());
    }

    /* 获取日志格式器 */
    LogFormatter::ptr Logger::getFormatter()
    {
        MutexType::Lock lock(m_mutex);
        return m_formatter;
    }

    /* 将 StdoutLogAppender 的配置转成YAML String */
    std::string StdoutLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    /* 将 FileLogAppender 的配置转成YAML String */
    std::string FileLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["filename"] = m_filename;
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        // 输出自己的专属模板，没有就不输出
        if (m_hasFormatter && m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    /* 将日志器的配置转成YAML String */
    std::string Logger::toYamlString()
    {
        MutexType::Lock loc(m_mutex);
        YAML::Node node;
        node["name"] = m_name;
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        for (auto &i : m_appenders)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(const std::string &filename) : m_filename(filename)
    {
        // 在初始化的时候就打开文件流可以避免在后续输出日志的时候，不断重复调用系统调用函数
        reopen();
    }

    LoggerManager::LoggerManager() : m_root(Logger::ptr(new Logger()))
    {
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender()));
        m_loggers[m_root->m_name] = m_root;
    }

    /* 获取日志器 */
    Logger::ptr LoggerManager::getLogger(const std::string &name)
    {
        MutexType::Lock lock(m_mutex);
        auto it = m_loggers.find(name);
        if (it != m_loggers.end())
        {
            return it->second;
        }
        // 如果不存咋则添加
        Logger::ptr logger(new Logger());
        logger->m_root = m_root; // 不要忘记赋值主日志器
        m_loggers[name] = logger;
        return logger;
    }

    /* 初始化 */
    void LoggerManager::init()
    {
    }

    /* 将所有的日志器配置成YAML */
    std::string LoggerManager::LoggerManager::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        for (auto &i : m_loggers)
        {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    /* 可能是配置文件用的 */
    struct LogAppenderDefine
    {
        int type = 0; // 1 File, 2 Stdout
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine &rhs) const
        {
            return type == rhs.type && level == rhs.level && formatter == rhs.formatter && file == rhs.file;
        }
    };

    struct LogDefine
    {
        std::string name;
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(LogDefine &rhs) const
        {
            return name == rhs.name && level == rhs.level && formatter == rhs.formatter && appenders == rhs.appenders;
        }

        // 可能是为了map
        bool operator<(LogDefine &rhs) const
        {
            return name < rhs.name;
        }

        bool isValid() const
        {
            return !name.empty();
        }
    };

    template <>
    class LexicalCast<std::string, LogDefine>
    {
    public:
        /* 将 string 中的数据通过YAML转换后，格式化为 LogDefine类型中的数据 */
        LogDefine operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            if (!node["name"].IsDefined())
            {
                // 输出错误
                std::cout << "log config error: name is null, " << node << std::endl;
                throw std::logic_error("log config name is null");
            }
            LogDefine ld;
            ld.name = node["name"].as<std::string>();
            // 如果有级别，使用 fromstring 将级别转为 int
            ld.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
            if (node["formatter"].IsDefined())
            {
                ld.formatter = node["formatter"].as<std::string>();
            }
            if (node["appenders"].IsDefined())
            {
                node["appenders"];
                for (size_t i = 0; i < node["appenders"].size(); ++i)
                {
                    auto tmp_appender = node["appenders"][i];
                    LogAppenderDefine lad;
                    if (tmp_appender["type"].IsDefined())
                    {
                        std::string type = tmp_appender["type"].as<std::string>();

                        if (type == "FileLogAppender") // 如果是文件
                        {
                            lad.type = 1;
                            if (!tmp_appender["file"].IsDefined())
                            {
                                // 可以直接向std::cout中输出node信息(已经格式为字符串)
                                std::cout << "log config error : fileappender file is null, " << tmp_appender << std::endl;
                                continue;
                            }
                            lad.file = tmp_appender["file"].as<std::string>();
                            if (tmp_appender["formatter"].IsDefined())
                            {
                                lad.formatter = tmp_appender["formatter"].as<std::string>();
                            }
                            lad.level = LogLevel::FromString(tmp_appender["level"].IsDefined() ? tmp_appender["level"].as<std::string>() : "");
                        }
                        else if (type == "StdoutLogAppender") // 如果是终端
                        {
                            lad.type = 2;
                            if (tmp_appender["formatter"].IsDefined())
                            {
                                lad.formatter = tmp_appender["formatter"].as<std::string>();
                            }
                            lad.level = LogLevel::FromString(tmp_appender["level"].IsDefined() ? tmp_appender["level"].as<std::string>() : "");
                        }
                        else
                        {
                            // 输出错误
                            std::cout << "log config error : apender type id invalid, " << tmp_appender << std::endl;
                            continue;
                        }
                    }
                    ld.appenders.push_back(lad);
                }
            }
            return ld;
        }
    };

    template <>
    class LexicalCast<LogDefine, std::string>
    {
        /* 将 LogDefine 中的数据通过YAML转换后，格式化为 string 类型中的数据 */
        std::string operator()(const LogDefine &ld)
        {
            YAML::Node node;
            if (ld.name.empty())
            {
                std::cout << "log config error: name is null" << std::endl;
                throw std::logic_error("log config name is null");
            }
            node["name"] = ld.name;
            if (ld.level != LogLevel::UNKNOW)
            {
                node["level"] = LogLevel::ToString(ld.level);
            }
            if (!ld.formatter.empty())
            {
                node["formatter"] = ld.formatter;
            }
            for (auto &i : ld.appenders)
            {
                YAML::Node append_node;
                if (i.type == 1)
                {
                    append_node["type"] = "FileLogAppender";
                    if (!i.file.empty())
                    {
                        append_node["file"] = i.file;
                    }
                }
                else if (i.type == 2)
                {
                    append_node["type"] = "StdoutLogAppender";
                }

                if (i.level != LogLevel::UNKNOW)
                {
                    append_node["level"] = LogLevel::ToString(i.level);
                }
                if (!i.formatter.empty())
                {
                    append_node["formatter"] = i.formatter;
                }

                node["appenders"].push_back(append_node);
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 配置信息没有写
}