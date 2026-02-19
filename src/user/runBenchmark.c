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
    bool compress, 
    int depth, 
    Solver type,
    const char* label
) {
    printf("----------------------------------------------------------------\n");
    printf("Benchmarking: %s\n", label);

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
               stones, sizePow, compress ? "True (B48)" : "False (B60)", 
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
        renderCacheStats(); 
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

    setMoveFunction(CLASSIC_MOVE);

    // --- 1. T32 MODES (Standard RAM) ---
    runTest(&context, 2, T32_B48, true, 0,   LOCAL_SOLVER, "MODE_ND_B48_T32");
    runTest(&context, 2, T32_B48, true, 999, LOCAL_SOLVER, "MODE_D_B48_T32");
    runTest(&context, 2, T32_B60, false, 0,   LOCAL_SOLVER, "MODE_ND_B60_T32");
    runTest(&context, 2, T32_B60, false, 999, LOCAL_SOLVER, "MODE_D_B60_T32");

    // --- 2. T16 MODES (High RAM) ---
    runTest(&context, 2, T16_B48, true, 0,   LOCAL_SOLVER, "MODE_ND_B48_T16");
    runTest(&context, 2, T16_B48, true, 999, LOCAL_SOLVER, "MODE_D_B48_T16");

    // --- 3. AVALANCHE ---
    setMoveFunction(AVALANCHE_MOVE);
    runTest(&context, 2, T32_B48, true, 999, LOCAL_SOLVER, "Avalanche (D_B48_T32)");

    // --- 4. 3-STONE COMPARISON ---
    setMoveFunction(CLASSIC_MOVE);
    runTest(&context, 3, 24, true, 0,  LOCAL_SOLVER, "3-Stone B48 (Size 24)");
    runTest(&context, 3, 29, false, 0, LOCAL_SOLVER, "3-Stone B60 (Size 29)");

    // --- 5. GLOBAL SOLVER ---
    // Note: Global solver tests imply sizePow=0 and compress=true
    setMoveFunction(CLASSIC_MOVE);
    runTest(&context, 2, 0, true, 0, GLOBAL_SOLVER, "GLOBAL SOLVER - Classic (2 Stones)");

    setMoveFunction(AVALANCHE_MOVE);
    runTest(&context, 1, 0, true, 0, GLOBAL_SOLVER, "GLOBAL SOLVER - Avalanche (1 Stone)");

    printf("----------------------------------------------------------------\n");
    printf("Benchmark Complete.\n");
}
