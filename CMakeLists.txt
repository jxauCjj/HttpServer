cmake_minimum_required(VERSION 3.24)

project(HttpServer)

set(CMAKE_CXX_STANDARD 17)  # C++标准版本

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) # 指定可执行文件输出路径

file(GLOB_RECURSE SOURCES "src/*.cpp")  # 递归遍历得到src下的所有.cpp源文件

add_executable(server ${SOURCES})

target_include_directories(server PUBLIC src) # 增加头文件路径

target_link_libraries(server mysqlclient pthread)   # 动态链接第三方库