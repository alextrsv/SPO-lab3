cmake_minimum_required(VERSION 3.19)
project(Lab3 C)

set(CMAKE_C_STANDARD 99)

add_executable(Lab3 main.c client.c server.c server.h message.h message.c)

target_link_libraries(Lab3 pthread)
target_link_libraries(Lab3 ncurses)