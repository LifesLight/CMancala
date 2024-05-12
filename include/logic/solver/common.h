#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */

// Macro to initialize variables and setup
#define INITIALIZE_VARS \
    const int windowSize = 1; \
    const int depthStep = 1; \
    int alpha = INT32_MIN + 1; \
    int beta = INT32_MAX; \
    int score; \
    int currentDepth = 1; \
    int bestMove = -1; \
    int windowMisses = 0; \
    clock_t start = clock(); \
    nodeCount = 0; \
    renderOutput("Thinking...", PLAY_PREFIX); \

// Macro for the iterative deepening loop
// 'negamaxCall' is the function call, 'additionalBreak' is the custom break conditions
#define ITERATIVE_DEEPENING_LOOP(negamaxCall, additionalBreak) \
    while (true) { \
        solved = true; \
        score = negamaxCall; \
        if (score > alpha && score < beta) { \
            currentDepth += depthStep; \
            additionalBreak; \
            if (currentDepth > context->config->depth && context->config->depth > 0) break; \
            if (((double)(clock() - start) / CLOCKS_PER_SEC) >= context->config->timeLimit && context->config->timeLimit > 0) break; \
        } else { \
            windowMisses++; \
            alpha = score - windowSize; \
            beta = score + windowSize; \
        } \
    } \
    double timeTaken = (double)(clock() - start) / CLOCKS_PER_SEC; \
    context->lastTime = timeTaken; \
    context->lastNodes = nodeCount; \
    if (windowMisses > currentDepth) { \
        char message[256]; \
        snprintf(message, sizeof(message), "[WARNING]: High window misses! (You may increase \"windowSize\" in algo.c)"); \
        renderOutput(message, PLAY_PREFIX); \
    } \
    context->lastMove = bestMove; \
    context->lastEvaluation = score; \
    context->lastDepth = currentDepth - 1; \
    context->lastSolved = solved;

// Macro for the function body with a placeholder for the negamax function call
#define NEGAMAX_ROOT_BODY(board, depth, distribution, negamaxCall) \
    renderOutput("Thinking...", CHEAT_PREFIX); \
    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2; \
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2; \
    int index = 5; \
    int score; \
    for (int8_t i = start; i >= end; i--) { \
        if (board->cells[i] == 0) { \
            distribution[index] = INT32_MIN; \
            index--; \
            continue; \
        } \
        Board boardCopy; \
        copyBoard(board, &boardCopy); \
        makeMoveFunction(&boardCopy, i); \
        int alpha = INT32_MIN + 1; \
        int beta = INT32_MAX; \
        if (board->color == boardCopy.color) { \
            score = negamaxCall; \
        } else { \
            score = -negamaxCall; \
        } \
        distribution[index] = score; \
        index--; \
    }
