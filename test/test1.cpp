#include <iostream>
#include <yaml-cpp/yaml.h>
#include <boost/lexical_cast.hpp>
int main()
{
    // 假设 node 包含了一些 YAML 数据
    YAML::Node node;
    node["appenders"]= "FileLogAppender";
    node["sfsdf"] = "example.log";
    std::cout << node << std::endl;
    return 0;
}
