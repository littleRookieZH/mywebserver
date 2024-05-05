#include "../../webs/webs.h"

static webs::Logger::ptr g_logger = WEBS_LOG_ROOT();

void test_timer() {
    WEBS_LOG_INFO(g_logger) << " test in timer";
    webs::IOManager iom(1);
    iom.addTimer(
        500, []() { WEBS_LOG_INFO(g_logger) << "  hello timer"; }, true);
}

int main() {
    test_timer();
    return 0;
}