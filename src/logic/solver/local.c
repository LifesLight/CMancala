/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/local.h"

static int LOCAL_negamax(Board *board, int alpha, int beta, const int depth, bool *solved) {
    if (processBoardTerminal(board)) {
        *solved = true;
        return board->color * getBoardEvaluation(board);
    }

    int cachedValue;
    int boundType;
    bool cachedSolved;
    if (getCachedValue(board, depth, &cachedValue, &boundType, &cachedSolved)) {
        if (boundType == EXACT_BOUND) {
            *solved = cachedSolved;
            return cachedValue;
        }
        if (boundType == LOWER_BOUND) alpha = max(alpha, cachedValue);
        else if (boundType == UPPER_BOUND) beta = min(beta, cachedValue);

        if (alpha >= beta) {
            *solved = cachedSolved;
            return cachedValue;
        }
    }

    if (depth == 0) {
        *solved = false;
        return board->color * getBoardEvaluation(board);
    }

    int reference = INT32_MIN;
    const int alphaOriginal = alpha;
    nodeCount++;
    bool nodeSolved = true;
    Board boardCopy;
    int score;

    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end   = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) continue;
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        bool childSolved;
        if (board->color == boardCopy.color) {
            score = LOCAL_negamax(&boardCopy, alpha, beta, depth - 1, &childSolved);
        } else {
            score = -LOCAL_negamax(&boardCopy, -beta, -alpha, depth - 1, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;

        reference = max(reference, score);
        alpha = max(alpha, reference);
        if (alpha >= beta) break;
    }

    if (reference <= alphaOriginal) boundType = UPPER_BOUND;
    else if (reference >= beta) boundType = LOWER_BOUND;
    else boundType = EXACT_BOUND;

    cacheNode(board, reference, boundType, depth, nodeSolved);
    *solved = nodeSolved;
    return reference;
}

int LOCAL_negamaxWithMove(Board *board, int *bestMove, int alpha, int beta, const int depth, bool *solved) {
    if (processBoardTerminal(board)) {
        *bestMove = -1; *solved = true;
        return board->color * getBoardEvaluation(board);
    }
    if (depth == 0) {
        *bestMove = -1; *solved = false;
        return board->color * getBoardEvaluation(board);
    }

    nodeCount++;
    int reference = INT32_MIN;
    int score;
    Board boardCopy;
    *bestMove = -1;
    bool nodeSolved = true;

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) continue;
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        bool childSolved;
        if (board->color == boardCopy.color) {
            score = LOCAL_negamax(&boardCopy, alpha, beta, depth - 1, &childSolved);
        } else {
            score = -LOCAL_negamax(&boardCopy, -beta, -alpha, depth - 1, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;

        if (score > reference) {
            reference = score;
            *bestMove = i;
        }

        alpha = max(alpha, reference);
        if (alpha >= beta) break;
    }
    *solved = nodeSolved;
    return reference;
}

void LOCAL_distributionRoot(Board *board, int *distribution, bool *solved, SolverConfig *config) {
    if (getCacheSize() == 0) startCache(NORMAL_CACHE_SIZE);

    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;
    int index = 5;
    int score;

    int depth = config->depth;
    if (config->depth == 0) {
        depth = MAX_DEPTH;
        setCacheNoDepth(config->allowCompressedCache);
    } else {
        setCacheNoDepth(false);
    }

    const int alpha = config->clip ? 0 : INT32_MIN + 1;
    const int beta  = config->clip ? 1 : INT32_MAX;

    bool nodeSolved = true;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            distribution[index] = INT32_MIN;
            index--;
            continue;
        }
        Board boardCopy;
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        bool childSolved;
        if (board->color == boardCopy.color) {
            score = LOCAL_negamax(&boardCopy, alpha, beta, depth, &childSolved);
        } else {
            score = -LOCAL_negamax(&boardCopy, -beta, -alpha, depth, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;

        if (config->clip && score > 1) score = 1;

        distribution[index] = score;
        index--;
    }
    *solved = nodeSolved;
    stepCache();
    resetCacheStats();
}

void LOCAL_aspirationRoot(Context* context, SolverConfig *config) {
    if (getCacheSize() == 0) startCache(NORMAL_CACHE_SIZE);

    const int depthStep = 1;
    int currentDepth = 1;
    bool oneShot = false;

    if (config->timeLimit == 0 && config->depth == 0) {
        currentDepth = MAX_DEPTH;
        oneShot = true;
        setCacheNoDepth(config->allowCompressedCache);
    } else {
        setCacheNoDepth(false);
    }

    int bestMove = -1;
    int score = 0;
    bool solved = false;

    const int windowSize = 1;
    int window = windowSize;
    int alpha = INT32_MIN + 1;
    int beta  = INT32_MAX;
    int windowMisses = 0;

    clock_t start = clock();
    nodeCount = 0;

    double* depthTimes = context->metadata.lastDepthTimes;
    if (depthTimes != NULL) {
        for (int i = 0; i < MAX_DEPTH; i++) depthTimes[i] = -1.0;
    }
    double lastTimeCaptured = 0.0;

    while (true) {
        solved = true;
        int currentAlpha = config->clip ? 0 : alpha;
        int currentBeta  = config->clip ? 1 : beta;

        score = LOCAL_negamaxWithMove(context->board, &bestMove, currentAlpha, currentBeta, currentDepth, &solved);

        int timeIndex = oneShot ? 1 : currentDepth;
        if (depthTimes != NULL) {
            double t = (double)(clock() - start) / CLOCKS_PER_SEC;
            depthTimes[timeIndex] = t - lastTimeCaptured;
            lastTimeCaptured = t;
        }

        stepCache();
        if (solved) break;
        if (config->depth > 0 && currentDepth >= config->depth) break;
        if (config->timeLimit > 0 && ((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit) break;
        if (oneShot) break;

        currentDepth += depthStep;

        if (!config->clip && !oneShot) {
            if (score <= alpha || score >= beta) {
                windowMisses++;
                window *= 2;
                alpha = score - window;
                beta  = score + window;
                continue;
            }
            window = windowSize;
        }
    }

    if (config->clip && score > 1) score = 1;

    context->metadata.lastTime = (double)(clock() - start) / CLOCKS_PER_SEC;
    context->metadata.lastNodes = nodeCount;
    context->metadata.lastMove = bestMove;
    context->metadata.lastEvaluation = score;
    context->metadata.lastDepth = currentDepth;
    context->metadata.lastSolved = solved;

    if (!config->clip && windowMisses > currentDepth) {
        renderOutput("[WARNING]: High window misses!", PLAY_PREFIX);
    }
    if (config->clip && score < 0) {
        renderOutput("[WARNING]: Clipped solver used in losing position!", CHEAT_PREFIX);
    }
    resetCacheStats();
}
