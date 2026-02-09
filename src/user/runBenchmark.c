/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "user/runBenchmark.h"

void runBenchmark() {
    printf("Starting Benchmark Mode...\n");

    // Setup Context
    Board board;
    Board lastBoard;
    Context context;
    memset(&board, 0, sizeof(Board));
    memset(&lastBoard, 0, sizeof(Board));
    memset(&context, 0, sizeof(Context));
    context.board = &board;
    context.lastBoard = &lastBoard;

    double timings[MAX_DEPTH];
    SolverConfig config;

    // --- RUN 1: LOCAL UNCLIPPED, 3 STONES, CLASSIC ---
    printf("Running: LOCAL SOLVER (Unclipped, 3 stones, Classic)...\n");
    setMoveFunction(CLASSIC_MOVE);
    startCache(BENCHMARK_CACHE_POW);

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 3);
    board.color = 1;

    config = (SolverConfig){
        .solver = LOCAL_SOLVER,
        .depth = 0,
        .timeLimit = 0,
        .clip = false
    };
    LOCAL_aspirationRoot(&context, &config);
    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/local_3_normal", timings);

    // --- RUN 2: LOCAL CLIPPED, 3 STONES, CLASSIC ---
    printf("Running: LOCAL SOLVER (Clipped, 3 stones, Classic)...\n");
    setMoveFunction(CLASSIC_MOVE);
    startCache(BENCHMARK_CACHE_POW);

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 3);
    board.color = 1;

    config.clip = true;
    LOCAL_CLIP_aspirationRoot(&context, &config);
    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/local_clipped_3_normal", timings);

    // --- RUN 3: GLOBAL UNCLIPPED, 2 STONES, CLASSIC ---
    printf("Running: GLOBAL SOLVER (Unclipped, 2 stones, Classic)...\n");
    setMoveFunction(CLASSIC_MOVE);
    // No cache reset for GLOBAL

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2);
    board.color = 1;

    config = (SolverConfig){
        .solver = GLOBAL_SOLVER,
        .depth = 0,
        .timeLimit = 0,
        .clip = false
    };
    GLOBAL_aspirationRoot(&context, &config);
    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/global_2_normal", timings);

    // --- RUN 4: GLOBAL CLIPPED, 2 STONES, CLASSIC ---
    printf("Running: GLOBAL SOLVER (Clipped, 2 stones, Classic)...\n");
    setMoveFunction(CLASSIC_MOVE);

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2);
    board.color = 1;

    config.clip = true;
    GLOBAL_CLIP_aspirationRoot(&context, &config);
    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/global_clipped_2_normal", timings);

    // --- RUN 5: LOCAL (UNCLIPPED), 2 STONES, AVALANCHE ---
    printf("Running: LOCAL SOLVER (Unclipped, 2 stones, Avalanche)...\n");
    setMoveFunction(AVALANCHE_MOVE);
    startCache(BENCHMARK_CACHE_POW);

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2);
    board.color = 1;

    config.clip = false;
    LOCAL_aspirationRoot(&context, &config);
    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/local_2_avalanche", timings);

    // --- RUN 6: LOCAL CLIPPED, 2 STONES, AVALANCHE ---
    printf("Running: LOCAL SOLVER (Clipped, 2 stones, Avalanche)...\n");
    setMoveFunction(AVALANCHE_MOVE);
    startCache(BENCHMARK_CACHE_POW);

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2);
    board.color = 1;

    config = (SolverConfig){
        .solver = LOCAL_SOLVER,
        .depth = 0,
        .timeLimit = 0,
        .clip = true
    };
    LOCAL_CLIP_aspirationRoot(&context, &config);
    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/local_clipped_2_avalanche", timings);

    printf("Benchmark complete.\n");
}
