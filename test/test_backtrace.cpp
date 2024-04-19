#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#define BT_BUF_SIZE 100
void print_backtrace() {
    void *bt_buffer[BT_BUF_SIZE];
    int bt_size = backtrace(bt_buffer, BT_BUF_SIZE);
    char **bt_strings = backtrace_symbols(bt_buffer, bt_size);
    printf("backtrace:\n");
    for (int i = 0; i < bt_size; i++) {
        printf("%s(MISSING)\n", bt_strings[i]);
    }
    free(bt_strings);
}
int func_c() {
    print_backtrace();
    return 0;
}
int func_b() {
    return func_c();
}
int func_a() {
    return func_b();
}
int main() {
    return func_a();
    return 0;
}