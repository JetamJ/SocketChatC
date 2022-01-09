cmake_minimum_required(VERSION 3.7)
project(POSsemka1 C)

set(CMAKE_C_STANDARD 99)

add_executable(POSsemka1 main.c)

target_link_libraries(POSsemka1 pthread)
