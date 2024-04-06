#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <functional>
#include <map>
#include <tuple>

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
                       std::string &thread_name, std::shared_ptr<Logger> logger, LogLevel::Level level)
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
        m_event->getSS();
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
        NameFormatItem(const std::string& str = "") {}
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
        FiberIdFormatItem(const std::string& str = "") {}
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
    XX(m, MessageFormatItem),       // m: 消息
    XX(p, LevelFormatItem),         // p: 日志级别
    XX(r, ElaspseFormatItem),       // r: 累计毫秒数
    XX(c, NameFormatItem),          // c: 日志名称
    XX(t, ThreadIdFormatItem),      // t: 线程id
    XX(n, NewLineFormatItem),       // n: 换行
    XX(d, DateTimeFormatItem),      // d: 时间
    XX(f, FilenameFormatItem),      // f: 文件名
    XX(l, LineFormatItem),          // l: 行号
    XX(T, TabFormatItem),           // T: Tab
    XX(F, FiberIdFormatItem),       // F: 协程id
    XX(N, ThreadNameFormatItem),    // N: 线程名称
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
}
