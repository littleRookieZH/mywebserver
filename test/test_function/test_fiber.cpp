#include <ucontext.h>
#include <stdio.h>
#include <unistd.h>

static int i = 0;
int main(void) {
    ucontext_t context;

    getcontext(&context);
    printf("%d\n", i++);
    setcontext(&context);
    return 0;
}