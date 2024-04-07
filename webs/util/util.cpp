#include "util.h"
#include <fstream>
namespace webs
{
    bool FSUtil::Mkdir(const std::string &dirname)
    {
    }
    
    /* 从给定的文件路径中提取出目录部分，并返回该目录路径 */
    std::string FSUtil::Dirname(const std::string &filename)
    {
        if (filename.empty())
        {
            return ".";
        }
        // 从后向前搜第一个符合的，找到返回下标，失败返回std::string::npos
        std::size_t ops = filename.rfind('/');
        if (ops == 0)
        { // 根目录
            return "/";
        }
        else if (ops == std::string::npos)
        { // 当前目录
            return ".";
        }
        else
        {
            return filename.substr(0, ops); // 返回目录路径
        }
    }

    bool FSUtil::OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode)
    {
        // 使用文件流打开文件，可以直接从文件流读数据或者写数据。std::ios_base::openmode 如果文件不存在就会创建
        ofs.open(filename.c_str(), mode);
        // 文件打开失败，先创建目录，再重新打开
        if (!ofs.is_open())
        {
            std::string dirname = Dirname(filename);
            Mkdir(dirname);
            ofs.open(filename.c_str(), mode);
        }
        return ofs.is_open();
    }
}
