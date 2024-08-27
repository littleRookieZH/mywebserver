#ifndef __WEBS_UTIL_H__
#define __WEBS_UTIL_H__

#include <vector>
#include <string>
#include <iostream>
#include <cxxabi.h>
#include <json/json.h>
#include <yaml-cpp/yaml.h>


/* 常用的工具类 */
namespace webs {
/* 获取当前线程的ID */
pid_t GetThreadId();

/* 返回当前协程的ID */
uint32_t GetFiberId();

/* 获取调用栈的信息 */
void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

/* 将调用栈信息全部输出到字符串中 */
std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");

/* 获取当前微秒数 */
uint64_t GetCurrentUS();

uint64_t GetCurrentMS();

std::string Time2Str(time_t ts = time(0), const std::string &format = "%Y-%m-%d %H:%M:%S");
class FSUtil {
public:
    static void ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix);
    static bool Mkdir(const std::string &dirname);
    static bool IsRunningPidfile(const std::string &pidfile);
    static bool Rm(const std::string &path);
    static bool Mv(const std::string &from, std::string &to);
    static bool Realpath(const std::string &path, std::string &rpath);
    static bool Symlink(const std::string &frm, const std::string &to);
    static bool Unlink(const std::string &filename, bool exist = false);
    static std::string Dirname(const std::string &filename);
    static std::string Basename(const std::string &filename);
    static bool OpenForRead(std::ifstream &ifs, const std::string &filename, std::ios_base::openmode mode);
    static bool OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode);
};


class TypeUtil {
public:
    static int8_t ToChar(const std::string& str);
    static int64_t Atoi(const std::string& str);
    static double Atof(const std::string& str);
    static int8_t ToChar(const char* str);
    static int64_t Atoi(const char* str);
    static double Atof(const char* str);
};


class StringUtil {
public:
    static std::string Format(const char *fmt, ...);
    static std::string Formatv(const char *fmt, va_list ap);

    static std::string UrlEncode(const std::string &str, bool space_as_plus = true);
    static std::string UrlDecode(const std::string &str, bool space_as_plus = true);

    static std::string Trim(const std::string &str, const std::string &delimit = "\t\r\n");
    static std::string TrimLeft(const std::string &str, const std::string &delimit = "\t\r\n");
    static std::string TrimRight(const std::string &str, const std::string &delimit = "\t\r\n");

    static std::string WStringToString(const std::wstring &ws);
    static std::string StringToString(const std::string &s);
};

bool YamlToJson(const YAML::Node& ynode, Json::Value& jnode);
bool JsonToYaml(const Json::Value& jnode, YAML::Node& ynode);

/**
     * 用于输出T的类型名称
     *
     */
template <class T>
const char *TypeToName() {
    // abi::__cxa_demangle：获取类型的完整名称；type可以返回类型的的名称但是可能随着编译器不同而不同。cxa类似于解码
    static const char *s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}

} // namespace webs

#endif
