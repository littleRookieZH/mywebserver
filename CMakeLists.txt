cmake_minimum_required(VERSION 3.0.0)
project(webs)

# add_executable(webserver main.cpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(.)
include_directories(/apps/webs/include)
option(BUILD_TEST "ON for complile test" OFF)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

#设置可执行文件集合 --- 使用变量LIB_SRC代替
set(LIB_SRC
    webs/config_module/config.cpp
    webs/config_module/env.cpp
    webs/coroutine_module/fiber.cpp
    webs/coroutine_module/hook.cpp
    webs/coroutine_module/scheduler.cpp
    webs/log_module/log.cpp
    webs/thread_module/thread.cpp
    webs/util_module/mutex.cpp
    webs/util_module/util.cpp
    webs/IO_module/timer.cpp
    webs/IO_module/iomanager.cpp
    webs/test_cmake/hello.cpp

    # webs/test_cmake/test.cpp
    )


# 定义一个库文件 webs
add_library(webs SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(webs)


find_library(PTHREAD pthread)

#设置库文件集合 --- 使用变量LIBS代替
set(LIBS
        webs
        pthread
        yaml-cpp
        )

# webs_add_executable(testmake "test/test_function/testmake.cpp" webs "${LIBS}")
webs_add_executable(test_scheduler "test/test_module/test_scheduler.cpp" webs "${LIBS}")
webs_add_executable(test_timer "test/test_module/test_timer.cpp" webs "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)