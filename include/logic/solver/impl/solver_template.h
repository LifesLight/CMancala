/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/impl/macros.h"

// --- Helper: Negamax ---

static int FN(negamax)(Board *board, int alpha, int beta, const int depth, bool *solved) {
    if (processBoardTerminal(board)) {
        *solved = true;
        return board->color * getBoardEvaluation(board);
    }

#if SOLVER_USE_CACHE
    int cachedValue, boundType;
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
#endif

    if (depth == 0) {
        *solved = false;
        return board->color * getBoardEvaluation(board);
    }

    int reference = INT32_MIN;

#if SOLVER_USE_CACHE
    const int alphaOriginal = alpha;
#endif

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
            score = FN(negamax)(&boardCopy, alpha, beta, depth - 1, &childSolved);
        } else {
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth - 1, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;

        reference = max(reference, score);
        alpha = max(alpha, reference);
        if (alpha >= beta) break;
    }

#if SOLVER_USE_CACHE
    int saveBoundType;
    if (reference <= alphaOriginal) saveBoundType = UPPER_BOUND;
    else if (reference >= beta) saveBoundType = LOWER_BOUND;
    else saveBoundType = EXACT_BOUND;

    cacheNode(board, reference, saveBoundType, depth, nodeSolved);
#endif

    *solved = nodeSolved;
    return reference;
}

// --- Helper: Negamax With Move (Root Helper) ---

static int FN(negamaxWithMove)(Board *board, int *bestMove, int alpha, int beta, const int depth, bool *solved) {
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
            score = FN(negamax)(&boardCopy, alpha, beta, depth - 1, &childSolved);
        } else {
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth - 1, &childSolved);
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

// --- Public: Distribution Root ---

void FN(distributionRoot)(Board *board, int *distribution, bool *solved, SolverConfig *config) {
    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;
    int index = 5;
    int score;

#if SOLVER_USE_CACHE
    // LOCAL logic: explicit if/else for depth and mode
    int depth = config->depth;
    if (config->depth == 0) {
        depth = MAX_DEPTH;
        setCacheMode(false, config->compressCache);
    } else {
        setCacheMode(true, config->compressCache);
    }
#else
    // GLOBAL logic: ternary, no cache call
    int depth = (config->depth == 0) ? MAX_DEPTH : config->depth;
#endif

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
            score = FN(negamax)(&boardCopy, alpha, beta, depth, &childSolved);
        } else {
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;

        if (config->clip && score > 1) score = 1;

        distribution[index] = score;
        index--;
    }
    *solved = nodeSolved;

#if SOLVER_USE_CACHE
    stepCache();
    resetCacheStats();
#endif
}

// --- Public: Aspiration Root ---

void FN(aspirationRoot)(Context* context, SolverConfig *config) {
    const int depthStep = 1;
    int currentDepth = 1;

#if SOLVER_USE_CACHE
    // LOCAL: OneShot logic + Cache Mode
    bool oneShot = false;
    if (config->timeLimit == 0 && config->depth == 0) {
        currentDepth = MAX_DEPTH;
        oneShot = true;
        setCacheMode(false, config->compressCache);
    } else {
        setCacheMode(true, config->compressCache);
    }
#else
    // GLOBAL: No OneShot, no cache calls, no depth override here
#endif

    startProgress(config, PLAY_PREFIX);

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
        bool searchValid = false;

        if (config->clip) {
            score = FN(negamaxWithMove)(context->board, &bestMove, 0, 1, currentDepth, &solved);
            searchValid = true;
        } 
#if SOLVER_USE_CACHE
        else if (oneShot) {
            // LOCAL only optimization
            score = FN(negamaxWithMove)(context->board, &bestMove, INT32_MIN + 1, INT32_MAX, currentDepth, &solved);
            searchValid = true;
        } 
#endif
        else {
            score = FN(negamaxWithMove)(context->board, &bestMove, alpha, beta, currentDepth, &solved);

            if (score > alpha && score < beta) {
                searchValid = true;
                window = windowSize;
                alpha = score - window;
                beta  = score + window;
            } else {
                windowMisses++;
                window *= 2;
                alpha = score - window;
                beta  = score + window;
            }
        }

#if SOLVER_USE_CACHE
        stepCache();
#endif

        if (searchValid) {
            int timeIndex = 
#if SOLVER_USE_CACHE
                oneShot ? 1 : 
#endif
                currentDepth;

            if (depthTimes != NULL) {
                double t = (double)(clock() - start) / CLOCKS_PER_SEC;
                if(timeIndex < MAX_DEPTH) depthTimes[timeIndex] = t - lastTimeCaptured;
                lastTimeCaptured = t;
            }

            updateProgress(currentDepth, bestMove, score, nodeCount);

            if (solved) break;
#if SOLVER_USE_CACHE
            if (oneShot) break;
#endif
            if (config->depth > 0 && currentDepth >= config->depth) break;
            if (config->timeLimit > 0 && ((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit) break;

            currentDepth += depthStep;
        }
    }

    finishProgress();

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

#if SOLVER_USE_CACHE
    resetCacheStats();
#endif
}
