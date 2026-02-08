/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/clipped/local.h"

#define PREFIX LOCAL_CLIP
#define IS_CLIPPED 1
#include "logic/solver/impl/local_template.h"
#undef PREFIX
#undef IS_CLIPPED

void LOCAL_CLIP_aspirationRoot(Context* context, SolverConfig *config) {
    // Cache checking
    if (getCacheSize() == 0) {
        renderOutput("Cache is disabled, starting with \"normal\" preset!", CHEAT_PREFIX);
        startCache(NORMAL_CACHE_SIZE);
    }

    const int depthStep = 1;

    int score;
    int currentDepth = 1;
    int bestMove = -1;
    bool solved = false;

    clock_t start = clock();
    nodeCount = 0;

    while (true) {
        score = LOCAL_CLIP_negamaxWithMove(context->board, &bestMove, currentDepth, &solved);
        currentDepth += depthStep;
        stepCache();
        if (solved) break;
        if (currentDepth > config->depth && config->depth > 0) break;
        if (((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit && config->timeLimit > 0) break;
    }

    double timeTaken = (double)(clock() - start) / CLOCKS_PER_SEC;
    context->metadata.lastTime = timeTaken;
    context->metadata.lastNodes = nodeCount;
    context->metadata.lastMove = bestMove;
    context->metadata.lastEvaluation = score;
    context->metadata.lastDepth = currentDepth - 1;
    context->metadata.lastSolved = solved;

    if (score < 0) {
        renderOutput("[WARNING]: Clipped best move calculators should not be used in losing positions!", CHEAT_PREFIX);
    }

    resetCacheStats();
}
