/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "user/runBenchmark.h"

void writeResults(double* t_unclipped_local, double* t_clipped_local,
                  double* t_unclipped_global, double* t_clipped_global) {
    FILE *fp = fopen("cmancala_benchmark.txt", "w");
    if (!fp) {
        printf("Error writing to file.\n");
        return;
    }

    // CSV header
    fprintf(fp, "Depth,Local_Unclipped,Local_Clipped,Global_Unclipped,Global_Clipped\n");

    // Find deepest depth with any data to avoid empty lines
    int deepest = -1;
    for (int i = 0; i < BENCH_DEPTH; i++) {
        if (t_unclipped_local[i] != -1.0 || t_clipped_local[i] != -1.0 ||
            t_unclipped_global[i] != -1.0 || t_clipped_global[i] != -1.0) {
            deepest = i;
        }
    }

    if (deepest == -1) {
        // no data at all
        fclose(fp);
        printf("No timing data to write.\n");
        return;
    }

    for (int i = 0; i <= deepest; i++) {
        fprintf(fp, "%d,%.10g,%.10g,%.10g,%.10g\n",
            i,
            t_unclipped_local[i], t_clipped_local[i],
            t_unclipped_global[i], t_clipped_global[i]);
    }

    fclose(fp);
    printf("Data written to 'cmancala_benchmark.txt'\n");
}

void runBenchmark(void) {
    printf("Starting Benchmark Mode...\n");
    setMoveFunction(CLASSIC_MOVE);

    // Initialize Cache (2^24)
    startCache(24);

    // 2. Setup Context
    Board board;
    Board lastBoard;
    Context context;

    memset(&board, 0, sizeof(Board));
    memset(&lastBoard, 0, sizeof(Board));
    memset(&context, 0, sizeof(Context));

    context.board = &board;
    context.lastBoard = &lastBoard;

    double timesUnclippedLocal[BENCH_DEPTH];
    double timesClippedLocal[BENCH_DEPTH];
    double timesUnclippedGlobal[BENCH_DEPTH];
    double timesClippedGlobal[BENCH_DEPTH];

    for (int i = 0; i < BENCH_DEPTH; i++) {
        timesUnclippedLocal[i] = -1.0;
        timesClippedLocal[i] = -1.0;
        timesUnclippedGlobal[i] = -1.0;
        timesClippedGlobal[i] = -1.0;
    }

    // --- RUN 1: LOCAL UNCLIPPED ---
    printf("Running: LOCAL SOLVER (Unclipped, 3 stones)...\n");
    memset(&board, 0, sizeof(Board));
    configBoard(&board, 3);
    board.color = 1;

    SolverConfig configUnclipped = {
        .solver = LOCAL_SOLVER,
        .depth = 0,
        .timeLimit = 0,
        .clip = false
    };

    LOCAL_aspirationRootBench(&context, &configUnclipped, timesUnclippedLocal);

    // --- RUN 2: LOCAL CLIPPED ---
    printf("Running: LOCAL SOLVER (Clipped, 3 stones)...\n");
    memset(&board, 0, sizeof(Board));
    configBoard(&board, 3);
    board.color = 1;

    SolverConfig configClipped = {
        .solver = LOCAL_SOLVER,
        .depth = 0,
        .timeLimit = 0,
        .clip = true
    };

    LOCAL_CLIP_aspirationRootBench(&context, &configClipped, timesClippedLocal);

    // --- RUN 3: GLOBAL UNCLIPPED ---
    printf("Running: GLOBAL SOLVER (Unclipped, 2 stones)...\n");
    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2); // 2 stones per cell for GLOBAL
    board.color = 1;

    SolverConfig configGlobalUnclipped = {
        .solver = GLOBAL_SOLVER,
        .depth = 0,
        .timeLimit = 0,
        .clip = false
    };

    // assumes GLOBAL_aspirationRootBench exists
    GLOBAL_aspirationRootBench(&context, &configGlobalUnclipped, timesUnclippedGlobal);

    // --- RUN 4: GLOBAL CLIPPED ---
    printf("Running: GLOBAL SOLVER (Clipped, 2 stones)...\n");
    memset(&board, 0, sizeof(Board));
    configBoard(&board, 2); // 2 stones per cell for GLOBAL
    board.color = 1;

    SolverConfig configGlobalClipped = {
        .solver = GLOBAL_SOLVER,
        .depth = 0,
        .timeLimit = 0,
        .clip = true
    };

    GLOBAL_CLIP_aspirationRootBench(&context, &configGlobalClipped, timesClippedGlobal);

    // --- FINISH ---
    writeResults(timesUnclippedLocal, timesClippedLocal,
                 timesUnclippedGlobal, timesClippedGlobal);

    printf("Benchmark complete.\n");
}
