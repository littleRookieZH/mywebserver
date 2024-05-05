#include <iostream>
extern "C" {
void sleep(int seconds) {
    std::cout << "Custom sleep function called with " << seconds << " seconds\n";
}
}