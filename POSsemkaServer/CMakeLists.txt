cmake_minimum_required(VERSION 3.7)
project(POSsemkaServer C)

set(CMAKE_C_STANDARD 99)

add_executable(POSsemkaServer main.c Vlakno.h Vlakno.c)

target_link_libraries(POSsemkaServer pthread)