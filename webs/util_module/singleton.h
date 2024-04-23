#ifndef __WEBS_SINGLETON_H__
#define __WEBS_SINGLETON_H__
#include <mutex>
#include <memory>

/* 单例模式的封装 */
namespace webs {
namespace {

template <class T, class X, int N>
T &GetInstanceX() {
    static T v;
    return v;
}

template <class T, class X, int N>
std::shared_ptr<T> GetInstancePtr() {
    static std::shared_ptr<T> v(new T);
    return v;
}

} // namespace

/**
 * @brief 单例模式封装类
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template <class T, class X = void, int N = 0>
class Singleton {
public:
    /**
     * @brief 返回单例裸指针
     */
    static T *GetInstance() {
        static T v;
        return &v; // 返回 LoggerManager
        // return &GetInstanceX<T, X, N>();
    }
};

/**
 * @brief 单例模式智能指针封装类
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template <class T, class X = void, int N = 0>
class SingletonPtr {
public:
    /**
     * @brief 返回单例智能指针
     */
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
        //return GetInstancePtr<T, X, N>();
    }
};
// template <typename T>
// class Singleton {
// public:
//     // 获取单例模式对象的引用，带参数
//     template <typename... Args>
//     static T &GetInstance(Args &&... args) {
//         static std::once_flag flag;
//         std::call_once(flag, [&]() { instance.reset(new T(std::forward<Args>(args)...)); });
//         return *instance;
//     }

// private:
//     Singleton() = default;
//     ~Singleton() = default;
//     Singleton(const Singleton &) = delete;
//     Singleton &operator=(const Singleton &) = delete;
//     static std::unique_ptr<T> instance;
// };
}; // namespace webs

#endif