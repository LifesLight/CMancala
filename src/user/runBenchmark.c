/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "user/runBenchmark.h"

static double currentTimeMs() {
    return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}

void runBenchmark() {
    printf("Starting Benchmark Mode...\n");

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

    typedef struct {
        const char* name;
        double timeMs;
    } BenchmarkSummary;

    BenchmarkSummary summary[64];
    int summaryCount = 0;
    double totalTime = 0.0;

#define ADD_SUMMARY(label, ms) \
    do { \
        summary[summaryCount++] = (BenchmarkSummary){label, ms}; \
        totalTime += ms; \
    } while (0)

    double start, elapsed;

    // ---------------------------------------------------------
    // 1. LOCAL SOLVER (Unclipped, 3 stones, Classic)
    // ---------------------------------------------------------
    printf("Running: LOCAL SOLVER (Unclipped, 3 stones, Classic)...\n");
    setMoveFunction(CLASSIC_MOVE);
    setCacheSize(BENCHMARK_CACHE_POW);

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 3);
    board.color = 1;

    // config: solver, depth, time, clip, compressedCache
    config = (SolverConfig){ LOCAL_SOLVER, 0, 0, false, true };

    start = currentTimeMs();
    LOCAL_aspirationRoot(&context, &config);
    elapsed = currentTimeMs() - start;

    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/local_3_normal", timings);
    ADD_SUMMARY("Local, Unclipped, 3 stones, Classic, Depth 0", elapsed);

    // ---------------------------------------------------------
    // 2. LOCAL SOLVER (Clipped, 3 stones, Classic)
    // ---------------------------------------------------------
    printf("Running: LOCAL SOLVER (Clipped, 3 stones, Classic)...\n");
    setMoveFunction(CLASSIC_MOVE);
    invalidateCache();

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 3);
    board.color = 1;

    config = (SolverConfig){ LOCAL_SOLVER, 0, 0, true, true };

    start = currentTimeMs();
    LOCAL_aspirationRoot(&context, &config);
    elapsed = currentTimeMs() - start;

    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/local_clipped_3_normal", timings);
    ADD_SUMMARY("Local, Clipped, 3 stones, Classic, Depth 0", elapsed);

    // ---------------------------------------------------------
    // 3. GLOBAL SOLVER (Unclipped, 2 stones, Classic)
    // ---------------------------------------------------------
    printf("Running: GLOBAL SOLVER (Unclipped, 2 stones, Classic)...\n");
    setMoveFunction(CLASSIC_MOVE);

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2);
    board.color = 1;

    config = (SolverConfig){ GLOBAL_SOLVER, 0, 0, false, true };

    start = currentTimeMs();
    GLOBAL_aspirationRoot(&context, &config);
    elapsed = currentTimeMs() - start;

    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/global_2_normal", timings);
    ADD_SUMMARY("Global, Unclipped, 2 stones, Classic, Depth 0", elapsed);

    // ---------------------------------------------------------
    // 4. GLOBAL SOLVER (Clipped, 2 stones, Classic)
    // ---------------------------------------------------------
    printf("Running: GLOBAL SOLVER (Clipped, 2 stones, Classic)...\n");
    setMoveFunction(CLASSIC_MOVE);

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2);
    board.color = 1;

    config = (SolverConfig){ GLOBAL_SOLVER, 0, 0, true, true };

    start = currentTimeMs();
    GLOBAL_aspirationRoot(&context, &config);
    elapsed = currentTimeMs() - start;

    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/global_clipped_2_normal", timings);
    ADD_SUMMARY("Global, Clipped, 2 stones, Classic, Depth 0", elapsed);

    // ---------------------------------------------------------
    // 5. LOCAL SOLVER (Unclipped, 2 stones, Avalanche)
    // ---------------------------------------------------------
    printf("Running: LOCAL SOLVER (Unclipped, 2 stones, Avalanche)...\n");
    setMoveFunction(AVALANCHE_MOVE);
    invalidateCache();

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2);
    board.color = 1;

    config = (SolverConfig){ LOCAL_SOLVER, 0, 0, false, true };

    start = currentTimeMs();
    LOCAL_aspirationRoot(&context, &config);
    elapsed = currentTimeMs() - start;

    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/local_2_avalanche", timings);
    ADD_SUMMARY("Local, Unclipped, 2 stones, Avalanche, Depth 0", elapsed);

    // ---------------------------------------------------------
    // 6. LOCAL SOLVER (Clipped, 2 stones, Avalanche)
    // ---------------------------------------------------------
    printf("Running: LOCAL SOLVER (Clipped, 2 stones, Avalanche)...\n");
    setMoveFunction(AVALANCHE_MOVE);
    invalidateCache();

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2);
    board.color = 1;

    config = (SolverConfig){ LOCAL_SOLVER, 0, 0, true, true };

    start = currentTimeMs();
    LOCAL_aspirationRoot(&context, &config);
    elapsed = currentTimeMs() - start;

    memcpy(timings, context.metadata.lastDepthTimes, MAX_DEPTH * sizeof(double));
    storeBenchmarkData("benchmark/local_clipped_2_avalanche", timings);
    ADD_SUMMARY("Local, Clipped, 2 stones, Avalanche, Depth 0", elapsed);

    // ---------------------------------------------------------
    // 7. LOCAL SOLVER (Unclipped, 3 stones, Classic, depth 40)
    // ---------------------------------------------------------
    printf("Running: LOCAL SOLVER (Unclipped, 3 stones, Classic, depth 40)...\n");
    setMoveFunction(CLASSIC_MOVE);
    invalidateCache();

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 3);
    board.color = 1;

    config = (SolverConfig){ LOCAL_SOLVER, 40, 0, false, true };

    start = currentTimeMs();
    LOCAL_aspirationRoot(&context, &config);
    elapsed = currentTimeMs() - start;

    ADD_SUMMARY("Local, Unclipped, 3 stones, Classic, Depth 40", elapsed);

    // ---------------------------------------------------------
    // 8. LOCAL SOLVER (Clipped, 3 stones, Classic, depth 40)
    // ---------------------------------------------------------
    printf("Running: LOCAL SOLVER (Clipped, 3 stones, Classic, depth 40)...\n");
    setMoveFunction(CLASSIC_MOVE);
    invalidateCache();

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 3);
    board.color = 1;

    config = (SolverConfig){ LOCAL_SOLVER, 40, 0, true, true };

    start = currentTimeMs();
    LOCAL_aspirationRoot(&context, &config);
    elapsed = currentTimeMs() - start;

    ADD_SUMMARY("Local, Clipped, 3 stones, Classic, Depth 40", elapsed);

    // ---------------------------------------------------------
    // 9. GLOBAL SOLVER (Unclipped, 2 stones, Classic, depth 30)
    // ---------------------------------------------------------
    printf("Running: GLOBAL SOLVER (Unclipped, 2 stones, Classic, depth 30)...\n");
    setMoveFunction(CLASSIC_MOVE);

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2);
    board.color = 1;

    config = (SolverConfig){ GLOBAL_SOLVER, 30, 0, false, true };

    start = currentTimeMs();
    GLOBAL_aspirationRoot(&context, &config);
    elapsed = currentTimeMs() - start;

    ADD_SUMMARY("Global, Unclipped, 2 stones, Classic, Depth 30", elapsed);

    // ---------------------------------------------------------
    // 10. GLOBAL SOLVER (Clipped, 2 stones, Classic, depth 30)
    // ---------------------------------------------------------
    printf("Running: GLOBAL SOLVER (Clipped, 2 stones, Classic, depth 30)...\n");
    setMoveFunction(CLASSIC_MOVE);

    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2);
    board.color = 1;

    config = (SolverConfig){ GLOBAL_SOLVER, 30, 0, true, true };

    start = currentTimeMs();
    GLOBAL_aspirationRoot(&context, &config);
    elapsed = currentTimeMs() - start;

    ADD_SUMMARY("Global, Clipped, 2 stones, Classic, Depth 30", elapsed);


    printf("\n=== BENCHMARK SUMMARY ===\n");

    for (int i = 0; i < summaryCount; i++) {
        printf("%s: %.0fms\n", summary[i].name, summary[i].timeMs);
    }

    printf("Total Time: %.0fms\n", totalTime);
    printf("(Depth 0 = infinite depth)\n");

    printf("Benchmark complete.\n");
}
