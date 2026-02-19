/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "user/runBenchmark.h"
#include "logic/solver/cache.h" 

static double currentTimeMs() {
    return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}

static void runTest(
    Context* context, 
    int stones, 
    int sizePow, 
    bool compress, 
    int depthConfig, 
    const char* label
) {
    printf("----------------------------------------------------------------\n");
    printf("Benchmarking: %s\n", label);
    printf("Config: Stones=%d, Cache=2^%d, Compress=%s, Mode=%s\n", 
           stones, sizePow, compress ? "True (B48)" : "False (B60)", 
           depthConfig == 0 ? "NODEPTH" : "DEPTH");

    setCacheSize(sizePow);

    memset(context->board, 0, sizeof(Board));
    configBoard(context->board, stones);
    context->board->color = 1;

    SolverConfig config = { LOCAL_SOLVER, depthConfig, 0, false, compress, false };

    double start = currentTimeMs();
    aspirationRoot_LOCAL(context, &config);
    double elapsed = currentTimeMs() - start;

    printf("Result: %.2f ms | Eval: %d | Nodes: %" PRIu64 "\n", elapsed, context->metadata.lastEvaluation, context->metadata.lastNodes);

    renderCacheStats(); 
}

// Separate helper for Global Solver tests (No Cache Stats)
static void runGlobalTest(
    Context* context,
    int stones,
    int sizePow,
    const char* label
) {
    printf("----------------------------------------------------------------\n");
    printf("Benchmarking: %s\n", label);
    printf("Config: Stones=%d, Cache=2^%d, GLOBAL SOLVER\n", stones, sizePow);

    setCacheSize(sizePow);

    memset(context->board, 0, sizeof(Board));
    configBoard(context->board, stones);
    context->board->color = 1;

    // Global solver config
    SolverConfig config = { GLOBAL_SOLVER, 0, 0, false, true, false };

    double start = currentTimeMs();
    aspirationRoot_GLOBAL(context, &config);
    double elapsed = currentTimeMs() - start;

    printf("Result: %.2f ms | Eval: %d | Nodes: %" PRIu64 "\n", elapsed, context->metadata.lastEvaluation, context->metadata.lastNodes);
}

void runBenchmark() {
    printf("Starting Coverage Benchmark...\n");

    Board board;
    Board lastBoard;
    Context context;

    memset(&board, 0, sizeof(Board));
    memset(&lastBoard, 0, sizeof(Board));
    memset(&context, 0, sizeof(Context));

    context.board = &board;
    context.lastBoard = &lastBoard;

    // Sizes based on index bits required for Tag size
    const int SIZE_T32_B48 = 24; 
    const int SIZE_T32_B60 = 29; 
    const int SIZE_T16_B48 = 33; 

    setMoveFunction(CLASSIC_MOVE);

    // ---------------------------------------------------------
    // 1. T32 MODES (Standard RAM)
    // ---------------------------------------------------------

    // MODE_ND_B48_T32
    runTest(&context, 2, SIZE_T32_B48, true, 0, "MODE_ND_B48_T32");

    // MODE_D_B48_T32
    runTest(&context, 2, SIZE_T32_B48, true, 999, "MODE_D_B48_T32");

    // MODE_ND_B60_T32 (Requires ~4GB)
    runTest(&context, 2, SIZE_T32_B60, false, 0, "MODE_ND_B60_T32");

    // MODE_D_B60_T32 (Requires ~4GB)
    runTest(&context, 2, SIZE_T32_B60, false, 999, "MODE_D_B60_T32");

    // ---------------------------------------------------------
    // 2. T16 MODES (High RAM: ~48GB)
    // ---------------------------------------------------------

    // MODE_ND_B48_T16
    runTest(&context, 2, SIZE_T16_B48, true, 0, "MODE_ND_B48_T16");

    // MODE_D_B48_T16
    runTest(&context, 2, SIZE_T16_B48, true, 999, "MODE_D_B48_T16");

    // ---------------------------------------------------------
    // 3. AVALANCHE (Functionality Check)
    // ---------------------------------------------------------
    setMoveFunction(AVALANCHE_MOVE);
    runTest(&context, 2, SIZE_T32_B48, true, 999, "Avalanche (D_B48_T32)");

    // ---------------------------------------------------------
    // 4. 3-STONE COMPARISON (Performance Check)
    // ---------------------------------------------------------
    setMoveFunction(CLASSIC_MOVE);

    // Compressed (B48)
    runTest(&context, 3, 24, true, 0, "3-Stone B48 (Size 24)");

    // Uncompressed (B60) - Must use Size 29+
    runTest(&context, 3, 29, false, 0, "3-Stone B60 (Size 29)");

    // ---------------------------------------------------------
    // 5. GLOBAL SOLVER
    // ---------------------------------------------------------
    setMoveFunction(CLASSIC_MOVE);
    runGlobalTest(&context, 2, 0, "GLOBAL SOLVER - Classic (2 Stones)");

    setMoveFunction(AVALANCHE_MOVE);
    runGlobalTest(&context, 1, 0, "GLOBAL SOLVER - Avalanche (1 Stone)");

    printf("----------------------------------------------------------------\n");
    printf("Benchmark Complete.\n");
}
