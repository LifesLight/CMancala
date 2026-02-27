/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "user/runBenchmark.h"

static double currentTimeMs() {
    return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}

static void runTest(
    Context* context, 
    int stones, 
    int sizePow, 
    CacheMode compress, 
    int depth, 
    Solver type,
    const char* label,
    MoveFunction moveFunction
) {
    printf("----------------------------------------------------------------\n");
    printf("Benchmarking: %s\n", label);

    // Synchronize global move mode
    setMoveFunction(moveFunction);

    if (type == LOCAL_SOLVER) {
        setCacheSize(sizePow);
        invalidateCache();
    }

    // Setup Board
    memset(context->board, 0, sizeof(Board));
    configBoard(context->board, stones);
    context->board->color = 1;

    SolverConfig config = {
        .solver = type, 
        .depth = depth, 
        .timeLimit = 0, 
        .progressBar = false, 
        .compressCache = compress, 
        .clip = false
    };

    // Print Config Details
    if (type == GLOBAL_SOLVER) {
        printf("Config: Stones=%d, Solver=GLOBAL\n", stones);
    } else {
        printf("Config: Stones=%d, Cache=2^%d, Compress=%s, Mode=%s\n", 
               stones, sizePow, compress == ALWAYS_COMPRESS ? "True (B48)" : "False (B60)", 
               depth == 0 ? "NODEPTH" : "DEPTH");
    }

    // Run & Time
    double start = currentTimeMs();
    aspirationRoot(context, &config);
    double elapsed = currentTimeMs() - start;

    printf("Result: %.2f ms | Eval: %d | Nodes: %" PRIu64 "\n", 
           elapsed, context->metadata.lastEvaluation, context->metadata.lastNodes);

    // Only render stats for local solver
    if (type == LOCAL_SOLVER) {
        renderCacheStats(false, false, false);
    }
}

void runBenchmark() {
    printf("Starting Coverage Benchmark...\n");
    Board board = {0};
    Board lastBoard = {0};
    Context context = {0};

    context.board = &board;
    context.lastBoard = &lastBoard;

    const int T32_B48 = 24; 
    const int T32_B60 = 29; 
    const int T16_B48 = 33; 

    // 1. T32 MODES (Standard RAM)
    runTest(&context, 2, T32_B48, ALWAYS_COMPRESS, 0,   LOCAL_SOLVER, "MODE_ND_B48_T32", CLASSIC_MOVE);
    runTest(&context, 2, T32_B48, ALWAYS_COMPRESS, 999, LOCAL_SOLVER, "MODE_D_B48_T32", CLASSIC_MOVE);
    runTest(&context, 2, T32_B60, NEVER_COMPRESS, 0,   LOCAL_SOLVER, "MODE_ND_B60_T32", CLASSIC_MOVE);

    // 2. T16 MODES (High RAM)
    runTest(&context, 2, T16_B48, ALWAYS_COMPRESS, 0,   LOCAL_SOLVER, "MODE_ND_B48_T16", CLASSIC_MOVE);

    // 3. AVALANCHE (Cannot use EGDB, tested here)
    runTest(&context, 2, T32_B48, ALWAYS_COMPRESS, 999, LOCAL_SOLVER, "Avalanche Local", AVALANCHE_MOVE);
    runTest(&context, 1, 0,       ALWAYS_COMPRESS, 0,   GLOBAL_SOLVER, "Avalanche Global", AVALANCHE_MOVE);

    // 4. GLOBAL CLASSIC
    runTest(&context, 2, 0, ALWAYS_COMPRESS, 0, GLOBAL_SOLVER, "Global Classic", CLASSIC_MOVE);

    // --- PHASE 2: EGDB SOLVERS ---
    // This covers: TT_EGDB_CLASSIC, EGDB_CLASSIC

    printf("----------------------------------------------------------------\n");
    printf("Generating EGDB for PGO Coverage...\n");

    generateEGDB(4);
    configureStoneCountEGDB(2);

    // 5. GLOBAL + EGDB
    // Should hit EGDB_CLASSIC path in algo.c
    runTest(&context, 2, 0, ALWAYS_COMPRESS, 0, GLOBAL_SOLVER, "EGDB_CLASSIC (Global 4 Stones)", CLASSIC_MOVE);

    configureStoneCountEGDB(3);

    // 6. LOCAL + EGDB
    // Should hit TT_EGDB_CLASSIC path in algo.c
    // Using a depth limit ensures we actually use the TT logic combined with EGDB probes
    runTest(&context, 3, T32_B48, ALWAYS_COMPRESS, 20, LOCAL_SOLVER, "TT_EGDB_CLASSIC (Local 4 Stones)", CLASSIC_MOVE);

    // Clean up
    freeEGDB();

    printf("----------------------------------------------------------------\n");
    printf("Benchmark Complete.\n");
}
