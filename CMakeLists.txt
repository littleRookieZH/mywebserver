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

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIR})
endif()


#设置可执行文件集合 --- 使用变量LIB_SRC代替
set(LIB_SRC
    webs/config_module/config.cpp
    webs/config_module/env.cpp

    webs/coroutine_module/fd_manager.cpp
    webs/coroutine_module/fiber.cpp
    webs/coroutine_module/hook.cpp
    webs/coroutine_module/scheduler.cpp

    webs/http_module/http.cpp
    webs/http_module/http_connection.cpp
    webs/http_module/http_parser.cpp
    webs/http_module/http_server.cpp
    webs/http_module/http_session.cpp
    webs/http_module/servlet.cpp
    webs/http_module/http11_parser.rl.cpp
    webs/http_module/httpclient_parser.rl.cpp
    # webs/http_module/status_servlet.cpp

    webs/log_module/log.cpp

    webs/util_module/mutex.cpp
    webs/util_module/util.cpp

    webs/io_module/timer.cpp
    webs/io_module/iomanager.cpp

    webs/net_module/address.cpp
    webs/net_module/socket.cpp
    webs/net_module/tcp_server.cpp

    webs/stream_module/socket_stream.cpp
    webs/stream_module/stream.cpp
    webs/stream_module/zlib_stream.cpp

    webs/thread_module/thread.cpp

    webs/util_module/bytearray.cpp
    webs/util_module/mutex.cpp
    webs/util_module/util.cpp

    webs/test_cmake/hello.cpp

    # webs/test_cmake/test.cpp
    )

    ragelmaker(webs/http_module/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/webs/http_module)
    ragelmaker(webs/http_module/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/webs/http_module)
    ragelmaker(webs/net_module/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/webs/net_module)
# 定义一个库文件 webs
add_library(webs SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(webs)


find_library(PTHREAD pthread)

#设置库文件集合 --- 使用变量LIBS代替
set(LIBS
        webs
        pthread
        yaml-cpp
        jsoncpp
        ${OPENSSL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${Boost_LIBRARIES}
        )

webs_add_executable(testmake "test/test_function/testmake.cpp" webs "${LIBS}")
webs_add_executable(test_http_server "test/test_module/test_http_server.cpp" webs "${LIBS}")
webs_add_executable(test_myhttp "test/test_module/test_myhttp.cpp" webs "${LIBS}")
# webs_add_executable(test_timer "test/test_module/test_timer.cpp" webs "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)