cmake_minimum_required(VERSION 4.2)
project(exe_formats)

set(CMAKE_CXX_STANDARD 20)

add_library(exe_formats STATIC
        include/exf/msf/definitions.h
        include/exf/io.h
        include/exf/types.h
        src/exf/dummy.cpp
        include/exf/msf/layout_controller.h
        include/exf/msf/msf.h
        include/exf/msf/stream.h
        src/exf/msf/msf.cpp
        src/exf/msf/stream.cpp
        src/exf/msf/layout_controller.cpp
        include/exf/meta.h)
target_include_directories(exe_formats PRIVATE include)

add_executable(test test.cpp)
target_link_libraries(test exe_formats)
target_include_directories(test PRIVATE include)