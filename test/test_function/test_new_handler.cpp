#include <iostream>
#include <new>

void outOfMem1() {
    std::cout << "Unable to alloc memory";
    throw std::bad_alloc();
}
void outOfMem2() {
    std::cout << "Unable to alloc memory";
    throw std::bad_alloc();
}
int main() {
    std::set_new_handler(outOfMem1);
    try {
        int *p = new int[1000000000000000000L];
    } catch (const std::bad_alloc &e) {
        std::cout << "Caught exception: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
