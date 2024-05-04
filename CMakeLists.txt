cmake_minimum_required(VERSION 3.20)
project(CMancala)

set(CMAKE_C_STANDARD 23)

set(SOURCE_FILES
    src/main.c
    src/logic/utility.c
    src/logic/board.c
    src/logic/render.c
    src/logic/algo.c
    src/menu/handleConfig.c
    src/menu/handleGame.c
    src/menu/handlePlaying.c
    src/menu/interface.c
)

include_directories(include)

add_executable(CMancala ${SOURCE_FILES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -flto -march=native -fno-stack-protector -Wall -Wextra -pedantic")
