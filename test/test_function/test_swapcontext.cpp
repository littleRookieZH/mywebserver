#include <stdio.h>
#include <ucontext.h>
#include <iostream>
#include <unistd.h>
static ucontext_t ctx[3];

static void
f1(void) {
    int t = 2;
    while (t--) {
        sleep(2);
        printf("%s\n", "f1");
    }
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
    ctx[1].uc_link = nullptr; // 将执行 return 0
    makecontext(&ctx[1], f1, 0);

    // getcontext(&ctx[2]);
    // ctx[2].uc_stack.ss_sp = stack2;
    // ctx[2].uc_stack.ss_size = sizeof(stack2);
    // ctx[2].uc_link = nullptr;
    // makecontext(&ctx[2], f2, 0);

    // 将当前 context 保存到 ctx[0]，切换到 ctx[2]
    if (swapcontext(&ctx[0], &ctx[1])) {
    } else {
        printf("sfsfsf");
    }

    return 0;
}