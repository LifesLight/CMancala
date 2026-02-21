cmake_minimum_required(VERSION 3.20)
project(Mancala)

set(MANCALA_VERSION "7.0" CACHE STRING "Current Version")

# --- 1. Define Core Shared Files ---
set(CORE_SOURCES
    src/main.c
    src/logic/utility.c
    src/logic/board.c
    src/logic/solver/algo.c
    src/logic/solver/solver.c
    src/logic/throughput.c
    src/logic/solver/cache.c
    src/user/render.c
    src/user/runBenchmark.c
    src/user/handleConfig.c
    src/user/handleGame.c
    src/user/handlePlaying.c
)

include_directories(include)

# --- 2. Platform Specific Configuration ---
if(EMSCRIPTEN)
    message(STATUS "Configuring for WebAssembly (Emscripten)")

    set(CMAKE_C_STANDARD 17)
    set(CMAKE_C_STANDARD_REQUIRED ON)

    set(UI_SOURCES src/web/interface.c)

    add_executable(Mancala ${CORE_SOURCES} ${UI_SOURCES})
    target_link_libraries(Mancala m)

    target_compile_definitions(Mancala PRIVATE MANCALA_VERSION=\"${MANCALA_VERSION}\")

    set_target_properties(Mancala PROPERTIES SUFFIX ".html")
    target_compile_options(Mancala PRIVATE -O3)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/web/shell.html ${CMAKE_CURRENT_BINARY_DIR}/shell.html @ONLY)

    # Base Emscripten Flags
    set(EM_FLAGS 
        "-O3"
        "-s INITIAL_MEMORY=134217728"
        "-s NO_EXIT_RUNTIME=1"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8','UTF8ToString']"
        "-s EXPORTED_FUNCTIONS=['_main','_malloc','_free']"
        "--shell-file ${CMAKE_CURRENT_BINARY_DIR}/shell.html"
    )

    string(JOIN " " EM_LINK_FLAGS_STR ${EM_FLAGS})
    set_target_properties(Mancala PROPERTIES LINK_FLAGS "${EM_LINK_FLAGS_STR}")

else()
    message(STATUS "Configuring for Native/CLI Build")

    set(CMAKE_C_STANDARD 23)
    set(CMAKE_C_COMPILER "gcc")

    set(UI_SOURCES src/user/interface.c)

    add_executable(Mancala ${CORE_SOURCES} ${UI_SOURCES})
    target_link_libraries(Mancala m)

    target_compile_definitions(Mancala PRIVATE MANCALA_VERSION=\"${MANCALA_VERSION}\")

    # --- PGO options ---
    option(ENABLE_PGO "Enable profile guided optimization" OFF)
    set(PGO_STAGE "generate" CACHE STRING "PGO stage: generate or use")

    message(STATUS "ENABLE_PGO=${ENABLE_PGO}")
    message(STATUS "PGO_STAGE=${PGO_STAGE}")

    set(GCC_COMMON_FLAGS "-O3 -march=native -fno-stack-protector -Wall -Wextra -pedantic")
    set(LTO_FLAG "-flto")

    if(NOT WIN32)
        set(BASE_OPT_FLAGS "${GCC_COMMON_FLAGS}")
    else()
        set(BASE_OPT_FLAGS "")
    endif()

    set(PGO_FLAGS "")

    if(ENABLE_PGO)
        if(PGO_STAGE STREQUAL "generate")
            set(PGO_DATA_DIR "${CMAKE_BINARY_DIR}/pgo")
            set(PGO_FLAGS "-fprofile-generate=${PGO_DATA_DIR}")
            set(OPT_FLAGS "${BASE_OPT_FLAGS}")
        elseif(PGO_STAGE STREQUAL "use")
            set(PGO_DATA_DIR "${CMAKE_BINARY_DIR}/pgo")
            set(PGO_FLAGS "-fprofile-use=${PGO_DATA_DIR} -fprofile-correction")
            set(OPT_FLAGS "${BASE_OPT_FLAGS} ${LTO_FLAG}")
        else()
            message(FATAL_ERROR "PGO_STAGE must be 'generate' or 'use'. Got: ${PGO_STAGE}")
        endif()
    else()
        set(OPT_FLAGS "${BASE_OPT_FLAGS} ${LTO_FLAG}")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPT_FLAGS} ${PGO_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OPT_FLAGS} ${PGO_FLAGS}")
endif()
