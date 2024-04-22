#include "hook.h"

namespace webs {

static thread_local bool t_hook_enable = false;
/* 设置线程的hook状态 */
void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}
} // namespace webs
