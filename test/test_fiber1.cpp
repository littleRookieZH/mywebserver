#include <stdio.h>
#include <unistd.h>
#include <ucontext.h>

void foo(void) {
    printf("foo\n");
}

int main(void) {
    ucontext_t context;
    char stack[1024];

    getcontext(&context);
    context.uc_stack.ss_sp = stack;
    context.uc_stack.ss_size = sizeof(stack);
    context.uc_link = NULL;
    makecontext(&context, foo, 0);
    // 执行foo后，程序结束
    setcontext(&context);
    printf("Hello world\n");
    sleep(1);

    return 0;
}