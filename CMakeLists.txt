cmake_minimum_required(VERSION 3.20)
project(CMancala)

set(CMAKE_CXX_STANDARD 23)

set(SOURCE_FILES
    src/main.c
)

add_executable(Mancala ${SOURCE_FILES})

set(CMAKE_CXX_FLAGS "-O3 -march=native -fno-stack-protector -Wall -Wextra -pedantic")
