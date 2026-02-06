/**
 * Copyright (c) Alexander Kurtz 2026
 */

 #include "logic/solver/full/local.h"

#define PREFIX LOCAL
#define IS_CLIPPED 0
#include "logic/solver/impl/local_template.h"
#undef PREFIX
#undef IS_CLIPPED

void LOCAL_aspirationRoot(Context* context, SolverConfig *config) {
    // Cache checking
    if (getCacheSize() == 0) {
        renderOutput("Cache is disabled, starting with \"normal\" preset!", CHEAT_PREFIX);
        startCache(NORMAL_CACHE_SIZE);
    }

    const int windowSize = 1;
    const int depthStep = 1;

    int alpha = INT32_MIN + 1;
    int beta = INT32_MAX;
    int score;
    int currentDepth = 1;
    int bestMove = -1;
    int windowMisses = 0;
    bool solved = false;

    clock_t start = clock();
    nodeCount = 0;

    while (true) {
        score = LOCAL_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth, &solved);
        if (score > alpha && score < beta) {
            currentDepth += depthStep;
            if (currentDepth > config->depth && config->depth > 0) break;
            if (((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit && config->timeLimit > 0) break;
        } else {
            windowMisses++;
            alpha = score - windowSize;
            beta = score + windowSize;
        }
    }

    double timeTaken = (double)(clock() - start) / CLOCKS_PER_SEC;
    context->metadata.lastTime = timeTaken;
    context->metadata.lastNodes = nodeCount;
    if (windowMisses > currentDepth) {
        char message[256];
        snprintf(message, sizeof(message), "[WARNING]: High window misses! (You may increase \"windowSize\" in algo.c)");
        renderOutput(message, PLAY_PREFIX);
    }
    context->metadata.lastMove = bestMove;
    context->metadata.lastEvaluation = score;
    context->metadata.lastDepth = currentDepth - 1;
    context->metadata.lastSolved = solved;

    stepCache();
}
