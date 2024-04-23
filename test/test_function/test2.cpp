
#include <stdio.h>

#include <string>
#include <iostream>

int main() {
    std::string s;
    s.resize(256);
    const char *str = "11(fsadfsa11+";
    int res = sscanf(str, "%*[^(]%*[^_]%255[^)+]", &s[0]);
    std::cout << s << std::endl;
    // return func_a();
    return 0;
}