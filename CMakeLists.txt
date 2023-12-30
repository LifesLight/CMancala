cmake_minimum_required(VERSION 3.20)
project(CMancala)

set(CMAKE_C_STANDARD 23)

set(SOURCE_FILES
    src/main.c
    src/board.c
    src/render.c
    src/algo.c
)

include_directories(include)

add_executable(Mancala ${SOURCE_FILES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -flto -march=native -fno-stack-protector -Wall -Wextra -pedantic")
