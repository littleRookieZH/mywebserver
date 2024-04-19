#include "util.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <sys/syscall.h>
#include <execinfo.h>
#include "../log_module/log.h"

namespace webs {

static webs::Logger::ptr g_logger = WEBS_LOG_NAME("system");

/*使用系统调用获取线程*/
pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

/* 查找指定路径下的后缀为subfix的所有常规文件路径；files是传出参数 */
void FSUtil::ListAllFile(std::vector<std::string> &files,
                         const std::string &path, const std::string &subfix) {
    // 文件能否打开
    if (access(path.c_str(), 0) != 0) {
        return;
    }
    // 能打开，获取目录流
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return;
    }
    // 读取目录项，并检查类型
    struct dirent *dp = nullptr;
    while ((dp = readdir(dir)) != nullptr) { // 读取文件项
        if (dp->d_type == DT_DIR) {          // 跳过 . 和 ..
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                continue;
            }
            ListAllFile(files, path + "/" + dp->d_name, subfix);
        } else if (dp->d_type == DT_REG) { // 判断是否是常规文件
            std::string filename(dp->d_name);
            if (subfix.empty()) { // 无后缀，则添加所有文件
                files.push_back(path + "/" + filename);
            } else if (filename.size() >= subfix
                                              .size()) { // 确定是否是包含后缀的常规文件，并添加文件路径
                if (filename.substr(filename.size() - subfix.size()) == subfix) {
                    files.push_back(path + "/" + filename);
                }
            }
        }
    }
    // 关闭目录流
    closedir(dir);
}

/* 从pid文件中的第一行读取pid，并判断pid是否存在(不一定在运行) */
bool FSUtil::IsRunningPidfile(const std::string &pidfile) {
    // 判断文件是否能打开
    std::ifstream ifs(pidfile);
    std::string line;
    // getline 如果是空字符也会被读取，此时line不为空
    if (!ifs || !std::getline(ifs, line)) {
        return false; // 文件没打开或者读取失败（空文件）
    }
    // 读取文件的第一行数据，并转为pid
    if (line.empty()) {
        return false; // 可能是空行
    }
    // 查找pid是否存在
    pid_t pid = atoi(line.c_str()); // 将字符串转化为整型
    // Kill发送信号值的如果信号值为0表示检查pid是否存在，但是这种方式不可靠
    if (kill(pid, 0) != 0) {
        return false;
    }
    return true;
}

/* 读取文件信息，st 是传出参数。成功返回0，失败返回-1 */
static int __lstat(const char *file, struct stat *st = nullptr) {
    struct stat lst;
    int ret = lstat(file, &lst);
    if (st) {
        *st = lst;
    }
    return ret;
}

/* 创建目录，成功返回0 失败返回-1*/
static int __mkdir(const char *dirname) {
    // 如果目录存在，返回0
    if (access(dirname, F_OK) == 0) {
        return 0;
    }
    // 不存在则创建
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

/* 给定目录路径，创建目录 */
bool FSUtil::Mkdir(const std::string &dirname) {
    // 如果当前目录存在，直接返回 true。
    if (__lstat(dirname.c_str()) == 0) {
        return true;
    }
    // 目录不存在，从最上级开始一级一级创建
    char *path = strdup(dirname.c_str());
    char *ptr = strchr(path + 1, '/'); // 从索引为1的字符开始搜索；原因：/XXX/
    // 依次查找 /
    do {
        for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
            *ptr = '\0';
            if (__mkdir(path) != 0) {
                break;
            }
        }

        if (ptr != nullptr) {
            break;
        } else if (__mkdir(path) != 0) // 最后一级可能还没有创建。/XXX/XXX
        {
            break;
        }
        free(path);
        return true;
    } while (0); // 使用do-while的原因是避免多次重复代码：free和false
    // 如果创建失败返回false,并释放内存
    free(path);
    return false;
}

/* 从给定的文件路径中提取出目录部分，并返回该目录路径 */
std::string FSUtil::Dirname(const std::string &filename) {
    if (filename.empty()) {
        return ".";
    }
    // 从后向前搜第一个符合的，找到返回下标，失败返回std::string::npos
    std::size_t ops = filename.rfind('/');
    if (ops == 0) { // 根目录
        return "/";
    } else if (ops == std::string::npos) { // 当前目录
        return ".";
    } else {
        return filename.substr(0, ops); // 返回目录路径
    }
}

/* 删除文件 */
bool FSUtil::Unlink(const std::string &filename, bool exist) {
    if (!exist && __lstat(filename.c_str())) { // 文件不存在或者无权限
        return false;                          // 这里修改了
    }
    return ::unlink(filename.c_str()) == 0;
}

/* 删除目录及目录中的文件 */
bool FSUtil::Rm(const std::string &path) {
    // 判断文件是否存在
    struct stat st;
    //// 返回值有所调整
    if (lstat(path.c_str(), &st)) { // 获取文件信息失败（如果lstat函数返回非零值，意味着文件不存在或者无法访问）
        return false;
    }
    // 不是一个目录，使用unlink删除文件
    if (!(st.st_mode & S_IFDIR)) { // 可能是一个链接、文件，将链接数减1
        return Unlink(path);
    }
    // 打开目录流，读取目录项
    DIR *dir = opendir(path.c_str());
    if (!dir) {
        return false;
    }
    bool ret = true;
    struct dirent *dp = nullptr;
    // 如果是目录，递归删除
    while (dp = readdir(dir)) {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
            continue;
        }
        std::string filename = path + "/" + dp->d_name;
        ret = Rm(filename);
        if (!ret) {
            break;
        }
    }
    // 关闭目录流
    closedir(dir);
    // 调用了系统的 rmdir 函数，尝试删除指定路径的目录。
    // 删除目录；多线程errno是安全的，但是目前还不清楚多线程是否安全
    if (errno != 0 || ::rmdir(path.c_str()) != 0) {
        ret = false;
    }
    return ret;
}

/* 更换目录或者文件名：这里不确定是否有问题 */
bool FSUtil::Mv(const std::string &from, std::string &to) { // 文件不存在
    if (__lstat(to.c_str()) && errno == ENOENT) {
        return rename(from.c_str(), to.c_str()) == 0;
    } else if (errno != 0) {
        return false;
    }
    // 存在就删除
    if (!Rm(to)) {
        return false;
    }
    return rename(from.c_str(), to.c_str()) == 0;
}

/* 查找文件的绝对路劲 */
bool FSUtil::Realpath(const std::string &path, std::string &rpath) {
    // 判断文件是否存在
    if (__lstat(path.c_str())) {
        return false;
    }
    // 获取指定路径的绝对路径
    char *buf = ::realpath(path.c_str(), NULL); // 第二个参数为null时，函数会使用malloc自动分配一块内存存放结果
    if (buf == nullptr) {
        return false;
    }
    std::string(buf).swap(rpath); // 交换地址信息，导致它可以交换一个局部变量
    free(buf);                    // 手动释放
    return true;
}

/* 这个函数的作用是尝试创建一个符号链接，如果目标路径已经存在，则先删除目标路径，
    然后创建符号链接。最终返回创建操作的结果，true 表示创建成功，false 表示创建失败。*/
bool FSUtil::Symlink(const std::string &frm, const std::string &to) {
    // 如果文件不存在
    if (__lstat(to.c_str()) && errno == ENOENT) {
        return ::symlink(frm.c_str(), to.c_str()) == 0;
    } else if (errno != 0) {
        return false;
    }
    // 文件存在，删除文件
    if (!Rm(to)) {
        return false;
    }
    return ::symlink(frm.c_str(), to.c_str()) == 0;
}

/* 从给定的文件路径中提取出文件名部分 */
std::string FSUtil::Basename(const std::string &filename) {
    if (filename.empty()) {
        return nullptr;
    }
    size_t pos = filename.rfind('/');
    if (pos == std::string::npos) {
        return filename;
    } else {
        return filename.substr(pos + 1);
    }
}

/* 以输入文件流的方式打开文件 */
bool FSUtil::OpenForRead(std::ifstream &ifs, const std::string &filename, std::ios_base::openmode mode) {
    ifs.open(filename, mode);
    return ifs.is_open();
}

/* 以输出流的方式打开文件，如果文件不存在会先创建 */
bool FSUtil::OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode) {
    // 使用文件流打开文件，可以直接从文件流读数据或者写数据。std::ios_base::openmode 如果文件不存在就会创建
    ofs.open(filename.c_str(), mode); // open重新打开文件流
    // 文件打开失败，先创建目录，再重新打开
    if (!ofs.is_open()) {
        std::string dirname = Dirname(filename);
        Mkdir(dirname);
        ofs.open(filename.c_str(), mode);
    }
    return ofs.is_open();
}

/* 将字符串格式的调用栈信息进行解码；去除一些字符 */
static std::string demangle(const char *str) {
    // sscanf
    // %*[^(]%*[^_] %255[^)+]
    int rs;
    size_t size = 0;
    int status = 0;
    std::string rt;
    rt.resize(255);
    // 解析的格式：必须要依次出现 ( _ 才行
    if (sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0]) == 1) {
        /* 读取 &rt[0] 中的符号名，不传入缓冲区，size传出参数，status传入参数 表示解码状态；v是系统分配的内存 */
        char *v = abi::__cxa_demangle(&rt[0], nullptr, &size, &status);
        if (v) {
            std::string result(v);
            free(v);
            return result;
        }
    }
    // 解析失败直接读取255个字符
    if (sscanf(str, "%255s", &rt[0]) == 1) {
        return rt;
    }
    return str;
}

/* 获取调用栈的信息 */
void Backtrace(std::vector<std::string> &bt, int size, int skip) {
    void **array = (void **)malloc(sizeof(void *) * size);

    // 使用Backtrace读取栈信息，使用backtrace_symbols将信息转为字符串
    size_t len = ::backtrace(array, size);
    char **strings = backtrace_symbols(array, len);
    // 校验strings是否为空
    if (strings == NULL) {
        WEBS_LOG_ERROR(g_logger) << "backtrace_symbols error";
        return;
    }
    // 再将字符串解析为std::string 格式
    for (size_t i = skip; i < len; ++i) {
        bt.push_back(demangle(strings[i]));
    }
    free(array);
    free(strings);
}

/* 将调用栈信息全部输出到字符串中 */
std::string BacktraceToString(int size, int skip, const std::string &prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for (auto &i : bt) {
        ss << prefix << i << std::endl;
    }
    return ss.str();
}

} // namespace webs
