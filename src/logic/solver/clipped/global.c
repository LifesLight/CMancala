/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/clipped/global.h"

static bool solved;

// Template Instantiation
#define PREFIX GLOBAL_CLIP
#define IS_CLIPPED 1
#include "logic/solver/impl/global_template.h"
#undef PREFIX
#undef IS_CLIPPED

void GLOBAL_CLIP_aspirationRoot(Context* context, SolverConfig *config) {
    const int depthStep = 1;
    int score;
    int currentDepth = 1;
    int bestMove = -1;
    clock_t start = clock();
    nodeCount = 0;

    double* depthTimes = context->metadata.lastDepthTimes;

    // Benchmark Init
    if (depthTimes != NULL) {
        for (int i = 0; i < MAX_DEPTH; i++) depthTimes[i] = -1.0;
    }
    double lastTimeCaptured = 0.0;

    while (true) {
        solved = true;
        score = GLOBAL_CLIP_negamaxWithMove(context->board, &bestMove, currentDepth);

        // Benchmark Record
        if (depthTimes != NULL && currentDepth < MAX_DEPTH) {
            double currentTime = (double)(clock() - start) / CLOCKS_PER_SEC;
            depthTimes[currentDepth] = currentTime - lastTimeCaptured;
            lastTimeCaptured = currentTime;
        }

        currentDepth += depthStep;
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
}
