cmake_minimum_required(VERSION 3.20)
project(CMancala)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_COMPILER "gcc")

set(SOURCE_FILES
    src/main.c
    src/logic/utility.c
    src/logic/board.c
    src/logic/algo.c
    src/user/render.c
    src/user/handleConfig.c
    src/user/handleGame.c
    src/user/handlePlaying.c
    src/user/interface.c
    src/logic/solver/global.c
    src/logic/solver/local.c
    src/logic/solver/clipped/local.c
    src/logic/solver/clipped/global.c
    src/logic/solver/troughput.c
    src/logic/solver/solveCache.c
)

include_directories(include)

add_executable(CMancala ${SOURCE_FILES})


# Optimization flags, windows doesn't need these since we are using Release mode
if(NOT WIN32)
  set(NO_WINDOWS_FLAGS "-O3 -flto -march=native -fno-stack-protector -Wall -Wextra -pedantic")
else()
  set(NO_WINDOWS_FLAGS "")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPTIMIZATION_FLAGS} ${NO_WINDOWS_FLAGS}")
