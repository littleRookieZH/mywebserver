#include <iostream>
#include <vector>

class Sample {
public:
    // Constructor
    Sample();

    // Destructor
    ~Sample();

    // Member function to demonstrate functionality
    void printMessage();

private:
    // Member variables
    std::vector<int> data;
};