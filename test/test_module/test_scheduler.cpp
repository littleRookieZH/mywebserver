#include "../../webs/webs.h"

static webs::Logger::ptr g_logger = WEBS_LOG_ROOT();

void test_fiber() {
    WEBS_LOG_INFO(g_logger) << " test in fiber";
    WEBS_LOG_INFO(g_logger) << " test in fiber";
    // sleep(1);
    // static int count = 5;
    // if (--count > 0) {
    //     webs::Scheduler::GetThis()->schedule(&test_fiber, webs::GetThreadId());
    // }
}

int main() {
    webs::Scheduler sc(1, false, "test");
    WEBS_LOG_INFO(g_logger) << "main thread ";
    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();
    return 0;
}