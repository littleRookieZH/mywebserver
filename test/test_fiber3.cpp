#include <stdio.h>
#include <ucontext.h>
#include <iostream>
static ucontext_t ctx[3];

static void
f1(void) {
    printf("start f1\n");
    // 将当前 context 保存到 ctx[1]，切换到 ctx[2]
    swapcontext(&ctx[1], &ctx[2]);
    printf("finish f1\n");
}

void swap() {
    if (swapcontext(&ctx[2], &ctx[1])) {
        std::cout << "error" << std::endl;
    }
    std::cout << "swapcontext success" << std::endl;
}

static void
f2(void) {
    printf("start f2\n");
    // 将当前 context 保存到 ctx[2]，切换到 ctx[1]
    swap();
    printf("finish f2\n");
}

int main(void) {
    char stack1[8192];
    char stack2[8192];

    getcontext(&ctx[1]);
    ctx[1].uc_stack.ss_sp = stack1;
    ctx[1].uc_stack.ss_size = sizeof(stack1);
    ctx[1].uc_link = &ctx[0]; // 将执行 return 0
    makecontext(&ctx[1], f1, 0);

    getcontext(&ctx[2]);
    ctx[2].uc_stack.ss_sp = stack2;
    ctx[2].uc_stack.ss_size = sizeof(stack2);
    ctx[2].uc_link = nullptr;
    makecontext(&ctx[2], f2, 0);

    // 将当前 context 保存到 ctx[0]，切换到 ctx[2]
    swapcontext(&ctx[0], &ctx[2]);
    printf("sfsfsf");
    return 0;
}