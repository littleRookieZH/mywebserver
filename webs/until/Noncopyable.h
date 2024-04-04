#ifndef _WEBS_NONCOPYABLE_H_
#define _WEBS_NONCOPYABLE_H_

namespace webs
{
    /* 不可拷贝对象封装 */
    class Noncopyable
    {
    public:
        Noncopyable(){};
        ~Noncopyable(){};

    private:
        Noncopyable(const Noncopyable &);
        Noncopyable &operator=(const Noncopyable &);
    };
};

#endif