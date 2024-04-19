#ifndef __WEBS_SINGLETON_H__
#define __WEBS_SINGLETON_H__
#include <mutex>
#include <memory>

/* 单例模式的封装 */
namespace webs {
template <typename T>
class Singleton {
public:
    // 获取单例模式对象的引用，带参数
    template <typename... Args>
    static T &GetInstance(Args &&... args) {
        static std::once_flag flag;
        std::call_once(flag, [&]() { instance.reset(new T(std::forward<Args>(args)...)); });
        return *instance;
    }

private:
    Singleton() = default;
    ~Singleton() = default;
    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;
    static std::unique_ptr<T> instance;
};
}; // namespace webs

#endif