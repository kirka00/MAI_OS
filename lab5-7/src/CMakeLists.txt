cmake_minimum_required(VERSION 3.10)
project(lab5_7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq cppzmq)

function(configure_target target_name source_file)
    add_executable(${target_name} ${source_file})
    target_include_directories(${target_name} PRIVATE ${ZMQ_INCLUDE_DIRS})
    target_link_libraries(${target_name} PRIVATE ${ZMQ_LIBRARIES})
    target_compile_options(${target_name} PRIVATE ${ZMQ_CFLAGS_OTHER})
    target_link_directories(${target_name} PRIVATE ${ZMQ_LIBRARY_DIRS})
endfunction()

configure_target(control_node main.cpp)
configure_target(computing_node computing_node.cpp)
