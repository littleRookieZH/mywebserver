#ifndef _WEBS_UTIL_H_
#define _WEBS_UTIL_H_

#include <vector>
#include <string>
#include <iostream>

/* 常用的工具类 */
namespace webs
{
    /* 获取当前线程的ID */
    pid_t GetThreadId();

    /* 返回当前协程的ID */
    uint32_t GetFiberId();

    /* 获取当前微秒数 */
    uint64_t GetCurrentUS();


    class FSUtil
    {
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

}

#endif
