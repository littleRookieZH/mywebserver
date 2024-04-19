#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

void foo(void) {
    printf("foo\n");
}

void bar(void) {
    printf("bar\n");
}

int main(void) {
    ucontext_t context1, context2;
    char stack1[1024];
    char stack2[1024];

    getcontext(&context1);
    context1.uc_stack.ss_sp = stack1;
    context1.uc_stack.ss_size = sizeof(stack1);
    context1.uc_link = NULL;
    makecontext(&context1, foo, 0);

    getcontext(&context2);
    context2.uc_stack.ss_sp = stack2;
    context2.uc_stack.ss_size = sizeof(stack2);
    context2.uc_link = &context1;
    makecontext(&context2, bar, 0);

    printf("Hello world\n");
    sleep(1);
    setcontext(&context2);

    return 0;
}