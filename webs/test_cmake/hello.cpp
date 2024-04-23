#include "hello.h"

// Constructor
Sample::Sample() {
    // Initialize data vector with some values
    for (int i = 0; i < 10; ++i) {
        data.push_back(i);
    }
}

// Destructor
Sample::~Sample() {
    // Destructor code here (if needed)
}

// Member function to print message
void Sample::printMessage() {
    std::cout << "Hello, I'm a Sample object!" << std::endl;
    std::cout << "Data vector contents:" << std::endl;
    // for (size_t i = 0; i < data.size(); ++i) {
    //     std::cout << data[i] << " ";
    // }
    std::cout << std::endl;
}
