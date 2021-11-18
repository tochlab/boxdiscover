cmake_minimum_required(VERSION 3.0)
project(boxdiscover C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_executable(boxdiscover main.c)
