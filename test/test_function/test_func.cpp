#include <fstream>
#include <iostream>

int main() {
    std::ofstream ofs;
    ofs.open("non_existent_file.txt", std::ios::in);

    if (!ofs) {
        std::cout << "打开文件失败\n";
    } else {
        std::cout << "打开文件成功\n";
    }

    if (!ofs.is_open()) {
        std::cout << "打开文件失败\n";
    } else {
        std::cout << "打开文件成功\n";
    }

    return 0;
}
