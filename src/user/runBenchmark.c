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

    // Setup Board
    setCacheSize(sizePow);
    memset(context->board, 0, sizeof(Board));
    configBoard(context->board, stones);
    context->board->color = 1;

    // Setup Config
    SolverConfig config = { type, depth, 0, false, compress, false };
    
    // Print Config Details
    if (type == GLOBAL_SOLVER) {
        printf("Config: Stones=%d, Cache=GLOBAL\n", stones);
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

    // --- 1. T32 MODES (Standard RAM) ---
    // Hits: LOCAL_CLASSIC
    runTest(&context, 2, T32_B48, ALWAYS_COMPRESS, 0,   LOCAL_SOLVER, "MODE_ND_B48_T32", CLASSIC_MOVE);
    runTest(&context, 2, T32_B48, ALWAYS_COMPRESS, 999, LOCAL_SOLVER, "MODE_D_B48_T32", CLASSIC_MOVE);
    runTest(&context, 2, T32_B60, NEVER_COMPRESS, 0,   LOCAL_SOLVER, "MODE_ND_B60_T32", CLASSIC_MOVE);
    runTest(&context, 2, T32_B60, NEVER_COMPRESS, 999, LOCAL_SOLVER, "MODE_D_B60_T32", CLASSIC_MOVE);

    // --- 2. T16 MODES (High RAM) ---
    // Hits: LOCAL_CLASSIC
    runTest(&context, 2, T16_B48, ALWAYS_COMPRESS, 0,   LOCAL_SOLVER, "MODE_ND_B48_T16", CLASSIC_MOVE);
    runTest(&context, 2, T16_B48, ALWAYS_COMPRESS, 999, LOCAL_SOLVER, "MODE_D_B48_T16", CLASSIC_MOVE);

    // --- 3. AVALANCHE ---
    // Hits: LOCAL_AVALANCHE
    runTest(&context, 2, T32_B48, ALWAYS_COMPRESS, 999, LOCAL_SOLVER, "Avalanche (D_B48_T32)", AVALANCHE_MOVE);

    // --- 4. 3-STONE COMPARISON ---
    // Hits: LOCAL_CLASSIC
    runTest(&context, 3, 24, ALWAYS_COMPRESS, 0,  LOCAL_SOLVER, "3-Stone B48 (Size 24)", CLASSIC_MOVE);
    runTest(&context, 3, 29, NEVER_COMPRESS, 0,   LOCAL_SOLVER, "3-Stone B60 (Size 29)", CLASSIC_MOVE);

    // --- 5. GLOBAL SOLVER ---
    // Note: Global solver tests imply sizePow=0 and compress=true
    // Hits: GLOBAL_CLASSIC
    runTest(&context, 2, 0, ALWAYS_COMPRESS, 0, GLOBAL_SOLVER, "GLOBAL SOLVER - Classic (2 Stones)", CLASSIC_MOVE);

    // Hits: GLOBAL_AVALANCHE
    runTest(&context, 1, 0, ALWAYS_COMPRESS, 0, GLOBAL_SOLVER, "GLOBAL SOLVER - Avalanche (1 Stone)", AVALANCHE_MOVE);

    printf("----------------------------------------------------------------\n");
    printf("Benchmark Complete.\n");
}
