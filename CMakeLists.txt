cmake_minimum_required(VERSION 3.20)
project(Mancala)

set(MANCALA_VERSION "9.1" CACHE STRING "Current Version")

# --- Core Shared Files ---
set(CORE_SOURCES
    src/main.c
    src/logic/utility.c
    src/logic/board.c
    src/logic/solver/algo.c
    src/logic/solver/cache.c
    src/user/render.c
    src/user/handleConfig.c
    src/user/handleGame.c
    src/user/handlePlaying.c
)

if(NOT EMSCRIPTEN)
    list(APPEND CORE_SOURCES src/logic/solver/egdb/core.c src/user/runBenchmark.c)
endif()

include_directories(include)

# --- Platform Specific Configuration ---
if(EMSCRIPTEN)
    message(STATUS "Configuring for WebAssembly (Emscripten)")

    # --- ZSTD Source Embedding ---
    include(FetchContent)
    FetchContent_Declare(
        zstd
        GIT_REPOSITORY https://github.com/facebook/zstd.git
        GIT_TAG        v1.5.5
    )
    FetchContent_GetProperties(zstd)
    if(NOT zstd_POPULATED)
        FetchContent_Populate(zstd)
    endif()
    file(GLOB ZSTD_SOURCES 
        "${zstd_SOURCE_DIR}/lib/common/*.c"
        "${zstd_SOURCE_DIR}/lib/compress/*.c"
        "${zstd_SOURCE_DIR}/lib/decompress/*.c"
        "${zstd_SOURCE_DIR}/lib/dictBuilder/*.c"
    )

    add_compile_definitions(WEB_BUILD)

    set(CMAKE_C_STANDARD 17)
    set(CMAKE_C_STANDARD_REQUIRED ON)

    set(UI_SOURCES 
        src/web/interface.c
        src/logic/solver/egdb/core.c
        src/logic/solver/egdb/memory/direct.c
    )

    add_executable(Mancala ${CORE_SOURCES} ${UI_SOURCES} ${ZSTD_SOURCES})
    target_link_libraries(Mancala m)

    # Add ZSTD include path and necessary defines
    target_include_directories(Mancala PRIVATE "${zstd_SOURCE_DIR}/lib")
    target_compile_definitions(Mancala PRIVATE 
        MANCALA_VERSION=\"${MANCALA_VERSION}\"
        EGDB_BACKEND_NAME=\"RAM\"
        ZSTD_DISABLE_ASM
        ZSTD_LIB_DEPRECATED=0
        XXH_NAMESPACE=ZSTD_
    )

    set_target_properties(Mancala PROPERTIES SUFFIX ".html")
    target_compile_options(Mancala PRIVATE -O3)

    # Configure HTML Template
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/web/shell.html ${CMAKE_CURRENT_BINARY_DIR}/shell.html @ONLY)
    
    # !!! ADDED: Copy CSS to build directory !!!
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/web/style.css ${CMAKE_CURRENT_BINARY_DIR}/style.css COPYONLY)

    # Base Emscripten Flags
    set(EM_FLAGS 
        "-O3"
        "-flto"
        "-s WASM=1"
        "-s ASSERTIONS=0"
        "-s INITIAL_MEMORY=268435456"
        "-s NO_EXIT_RUNTIME=1"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s MAXIMUM_MEMORY=2684354560"
        "-s ENVIRONMENT=web"
        "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8','UTF8ToString']"
        # Removed explicit EXPORTED_FUNCTIONS to let KEEPALIVE work
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

    # --- Feature Options ---
    option(ENABLE_LZ4 "Enable LZ4 compression for EGDB" OFF)
    option(ENABLE_MMAP "Enable memory-mapped files for EGDB" OFF)
    option(ENABLE_PGO "Enable profile guided optimization" OFF)
    option(ENABLE_PROFILING "Enable debug symbols for perf analysis" OFF)

    # --- EGDB Backend Configuration ---
    if(ENABLE_LZ4)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(lz4 IMPORTED_TARGET liblz4)

        if(lz4_FOUND)
            message(STATUS "EGDB Backend: LZ4")
            target_link_libraries(${PROJECT_NAME} PkgConfig::lz4)
            target_compile_definitions(${PROJECT_NAME} PRIVATE LZ4)
            target_compile_definitions(${PROJECT_NAME} PRIVATE EGDB_BACKEND_NAME=\"LZ4\")
            target_sources(Mancala PRIVATE src/logic/solver/egdb/memory/lz4.c)
        else()
            message(WARNING "LZ4 enabled but not found. Falling back.")
            set(ENABLE_LZ4 OFF)
        endif()
    endif()

    if(NOT ENABLE_LZ4)
        if(ENABLE_MMAP)
            message(STATUS "EGDB Backend: MMAP")
            target_compile_definitions(${PROJECT_NAME} PRIVATE EGDB_BACKEND_NAME=\"MMAP\")
            target_sources(Mancala PRIVATE src/logic/solver/egdb/memory/mmap.c)
        else()
            message(STATUS "EGDB Backend: Direct RAM")
            target_compile_definitions(${PROJECT_NAME} PRIVATE EGDB_BACKEND_NAME=\"RAM\")
            target_sources(Mancala PRIVATE src/logic/solver/egdb/memory/direct.c)
        endif()
    endif()

    # --- Optimization / PGO ---
    set(PGO_STAGE "generate" CACHE STRING "PGO stage: generate or use")

    message(STATUS "ENABLE_PGO=${ENABLE_PGO}")
    message(STATUS "ENABLE_PROFILING=${ENABLE_PROFILING}")
    if(ENABLE_PGO)
        message(STATUS "PGO_STAGE=${PGO_STAGE}")
    endif()

    set(GCC_COMMON_FLAGS "-O3 -march=native -fno-stack-protector -Wall -Wextra -pedantic")

    if(ENABLE_PROFILING)
        set(GCC_COMMON_FLAGS "${GCC_COMMON_FLAGS} -g -fno-omit-frame-pointer")
    endif()

    set(LTO_FLAG "-flto=auto")

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
