#ifndef __WEBS_NONCOPYABLE_H__
#define __WEBS_NONCOPYABLE_H__

namespace webs {
/* 不可拷贝对象封装 */
class Noncopyable {
public:
    Noncopyable(){};
    ~Noncopyable(){};
    Noncopyable(const Noncopyable &) = delete;
    Noncopyable &operator=(const Noncopyable &) = delete;
};
}; // namespace webs

#endif