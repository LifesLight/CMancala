/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/global.h"

bool solved;

// Template Instantiation
#define PREFIX GLOBAL
#define IS_CLIPPED 0
#include "logic/solver/impl/global_template.h"
#undef PREFIX
#undef IS_CLIPPED

void GLOBAL_aspirationRoot(Context* context, SolverConfig *config) {
    const int windowSize = 1;
    const int depthStep = 1;
    int alpha = INT32_MIN + 1;
    int beta = INT32_MAX;
    int score;
    int currentDepth = 1;
    int bestMove = -1;
    int windowMisses = 0;
    clock_t start = clock();
    nodeCount = 0;
    while (true) {
        solved = true;
        score = GLOBAL_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth);
        if (score > alpha && score < beta) {
            currentDepth += depthStep;
            if (solved) break;
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
}
