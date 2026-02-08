cmake_minimum_required(VERSION 3.20)
project(Mancala)

set(CMAKE_C_STANDARD 23)
# default compiler name (user can override with -DCMAKE_C_COMPILER)
set(CMAKE_C_COMPILER "gcc")

set(SOURCE_FILES
    src/main.c
    src/logic/utility.c
    src/logic/board.c
    src/logic/solver/algo.c
    src/user/render.c
    src/user/handleConfig.c
    src/user/handleGame.c
    src/user/handlePlaying.c
    src/user/interface.c
    src/user/runBenchmark.c
    src/logic/solver/full/global.c
    src/logic/solver/full/local.c
    src/logic/solver/clipped/local.c
    src/logic/solver/clipped/global.c
    src/logic/throughput.c
    src/logic/solver/solveCache.c
)

include_directories(include)

add_executable(Mancala ${SOURCE_FILES})
target_link_libraries(Mancala m)

# --- PGO options ---
option(ENABLE_PGO "Enable profile guided optimization" OFF)
set(PGO_STAGE "generate" CACHE STRING "PGO stage: generate or use")

# Show status
message(STATUS "ENABLE_PGO=${ENABLE_PGO}")
message(STATUS "PGO_STAGE=${PGO_STAGE}")

# --- Optimization flags ---
set(GCC_COMMON_FLAGS "-O3 -march=native -fno-stack-protector -Wall -Wextra -pedantic")
set(LTO_FLAG "-flto")

if(NOT WIN32)
  # base flags (we will combine as needed below)
  set(BASE_OPT_FLAGS "${GCC_COMMON_FLAGS}")
else()
  set(BASE_OPT_FLAGS "")
endif()

# PGO flags default empty
set(PGO_FLAGS "")

if(ENABLE_PGO)
  if(PGO_STAGE STREQUAL "generate")
    # instrumented build. write profile data to build directory ./pgo
    set(PGO_DATA_DIR "${CMAKE_BINARY_DIR}/pgo")
    set(PGO_FLAGS "-fprofile-generate=${PGO_DATA_DIR}")
    # do not enable LTO for generate stage
    set(OPT_FLAGS "${BASE_OPT_FLAGS}")
  elseif(PGO_STAGE STREQUAL "use")
    # use collected profile. read from same directory.
    set(PGO_DATA_DIR "${CMAKE_BINARY_DIR}/pgo")
    set(PGO_FLAGS "-fprofile-use=${PGO_DATA_DIR} -fprofile-correction")
    # enable LTO in use stage for best results
    set(OPT_FLAGS "${BASE_OPT_FLAGS} ${LTO_FLAG}")
  else()
    message(FATAL_ERROR "PGO_STAGE must be 'generate' or 'use'. Got: ${PGO_STAGE}")
  endif()
else()
  set(OPT_FLAGS "${BASE_OPT_FLAGS} ${LTO_FLAG}")
endif()

# apply flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPT_FLAGS} ${PGO_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OPT_FLAGS} ${PGO_FLAGS}")

# friendly info
message(STATUS "C flags: ${CMAKE_C_FLAGS}")
message(STATUS "Linker flags: ${CMAKE_EXE_LINKER_FLAGS}")
