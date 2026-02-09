/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/full/global.h"

static bool solved;

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

    double* depthTimes = context->metadata.lastDepthTimes;

    // Benchmark Init
    if (depthTimes != NULL) {
        for (int i = 0; i < MAX_DEPTH; i++) depthTimes[i] = -1.0;
    }
    double lastTimeCaptured = 0.0;

    int window = windowSize;

    while (true) {
        solved = true;
        score = GLOBAL_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth);
        if (score > alpha && score < beta) {
            // Benchmark Record
            if (depthTimes != NULL && currentDepth < MAX_DEPTH) {
                double currentTime = (double)(clock() - start) / CLOCKS_PER_SEC;
                depthTimes[currentDepth] = currentTime - lastTimeCaptured;
                lastTimeCaptured = currentTime;
            }

            currentDepth += depthStep;
            window = windowSize;
            if (solved) break;
            if (currentDepth > config->depth && config->depth > 0) break;
            if (((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit && config->timeLimit > 0) break;
        } else {
            windowMisses++;
            window *= 2;
        }

        alpha = score - window;
        beta  = score + window;
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
