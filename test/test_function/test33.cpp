#include <iostream>
#include <iomanip>

int main() {
    int key = 42;
    std::stringstream ss;
    ss << std::setw(30) << std::right << key << ": ";
    std::cout << ss.str() << std::endl;
    return 0;
}