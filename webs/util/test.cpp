#include <iostream>
#include <functional>
#include <vector>

class ConfigVar
{
public:
    using OnChangeCb = std::function<void(int oldVal, int newVal)>;

    // 设置回调函数的值，并执行
    void setValue(int newVal)
    {
        if (newVal != m_val)
        {
            for (const auto &cb : m_cbs)
            {
                cb(m_val, newVal);
            }
            m_val = newVal;
        }
    }
    // 添加回调函数
    void addCallback(OnChangeCb cb)
    {
        m_cbs.push_back(cb);
    }

private:
    int m_val = 0;
    std::vector<OnChangeCb> m_cbs;
};
void func(int a, int b) { std::cout << a << " " << b << std::endl; }
int main()
{
    ConfigVar var;
    
    var.addCallback(func);

    var.setValue(10); // Output: Value changed from 0 to 10
    var.setValue(20); // Output: Value changed from 10 to 20

    return 0;
}
