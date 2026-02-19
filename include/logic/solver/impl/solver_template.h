/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/impl/macros.h"
#include "logic/utility.h"

#if !SOLVER_USE_CACHE
static bool solved;
#endif

// --- Helper: Key ---

static inline int FN(key)(Board* board, int color1) {
    int res = color1 * getBoardEvaluation(board);
    if(color1 == board->color) res += 1000;
    return res;
}

// --- Helper: Negamax ---

#if SOLVER_USE_CACHE
static int FN(negamax)(Board *board, int alpha, int beta, const int depth, bool *solved) {
#else
static int FN(negamax)(Board *board, int alpha, int beta, const int depth) {
#endif
    if (processBoardTerminal(board)) {
#if SOLVER_USE_CACHE
        *solved = true;
#endif
        return board->color * getBoardEvaluation(board);
    }

#if SOLVER_USE_CACHE
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
#endif

    if (depth == 0) {
#if SOLVER_USE_CACHE
        *solved = false;
#else
        solved = false;
#endif
        return board->color * getBoardEvaluation(board);
    }

#if SOLVER_USE_CACHE
    int reference = INT32_MIN;
    const int alphaOriginal = alpha;
    nodeCount++;
    bool nodeSolved = true;
    int score;
#else
    nodeCount++;
    int reference = INT32_MIN;
    int score;
#endif

    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end   = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;

    Board allMoves[6];
    int order[6];
    int keys[6];
    int valid = 0;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) continue;

        copyBoard(board, &allMoves[valid]);
        makeMoveFunction(&allMoves[valid], i);

        keys[valid] = FN(key)(&allMoves[valid], board->color);
        order[valid] = valid;
        valid++;
    }

    // Sort valid moves descending by evaluation key
    for (int i = 0; i < valid; i++) {
        int best = i;
        int best_key = keys[order[i]];

        for (int j = i + 1; j < valid; j++) {
            int k = keys[order[j]];
            if (k > best_key) {
                best_key = k;
                best = j;
            }
        }

        if (best != i) {
            int temp = order[i];
            order[i] = order[best];
            order[best] = temp;
        }

        Board *boardCopy = &allMoves[order[i]];

#if SOLVER_USE_CACHE
        bool childSolved;
        if (board->color == boardCopy->color) {
            score = FN(negamax)(boardCopy, alpha, beta, depth - 1, &childSolved);
        } else {
            score = -FN(negamax)(boardCopy, -beta, -alpha, depth - 1, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;
#else
        if (board->color == boardCopy->color) {
            score = FN(negamax)(boardCopy, alpha, beta, depth - 1);
        } else {
            score = -FN(negamax)(boardCopy, -beta, -alpha, depth - 1);
        }
#endif

        reference = max(reference, score);
        alpha = max(alpha, reference);

        if (alpha >= beta) break;
    }

#if SOLVER_USE_CACHE
    if (reference <= alphaOriginal) boundType = UPPER_BOUND;
    else if (reference >= beta) boundType = LOWER_BOUND;
    else boundType = EXACT_BOUND;

    cacheNode(board, reference, boundType, depth, nodeSolved);
    *solved = nodeSolved;
#endif
    return reference;
}

// --- Helper: Negamax With Move (Root Helper) ---

#if SOLVER_USE_CACHE
int FN(negamaxWithMove)(Board *board, int *bestMove, int alpha, int beta, const int depth, bool *solved) {
#else
int FN(negamaxWithMove)(Board *board, int *bestMove, int alpha, int beta, const int depth) {
#endif
    if (processBoardTerminal(board)) {
#if SOLVER_USE_CACHE
        *bestMove = -1; *solved = true;
#else
        *bestMove = -1;
#endif
        return board->color * getBoardEvaluation(board);
    }
    if (depth == 0) {
#if SOLVER_USE_CACHE
        *bestMove = -1; *solved = false;
#else
        solved = false;
        *bestMove = -1;
#endif
        return board->color * getBoardEvaluation(board);
    }

    nodeCount++;
    int reference = INT32_MIN;
    int score;
#if SOLVER_USE_CACHE
    *bestMove = -1;
    bool nodeSolved = true;
#else
    *bestMove = -1;
#endif

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    Board allMoves[6];
    int moves[6];
    int order[6];
    int keys[6];
    int valid = 0;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) continue;

        copyBoard(board, &allMoves[valid]);
        makeMoveFunction(&allMoves[valid], i);

        keys[valid] = FN(key)(&allMoves[valid], board->color);
        moves[valid] = i;  // Capture the original hole index safely
        order[valid] = valid;
        valid++;
    }

    // Sort valid moves descending by evaluation key
    for (int i = 0; i < valid; i++) {
        int best = i;
        int best_key = keys[order[i]];

        for (int j = i + 1; j < valid; j++) {
            int k = keys[order[j]];
            if (k > best_key) {
                best_key = k;
                best = j;
            }
        }

        if (best != i) {
            int temp = order[i];
            order[i] = order[best];
            order[best] = temp;
        }

        Board *boardCopy = &allMoves[order[i]];

#if SOLVER_USE_CACHE
        bool childSolved;
        if (board->color == boardCopy->color) {
            score = FN(negamax)(boardCopy, alpha, beta, depth - 1, &childSolved);
        } else {
            score = -FN(negamax)(boardCopy, -beta, -alpha, depth - 1, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;
#else
        if (board->color == boardCopy->color) {
            score = FN(negamax)(boardCopy, alpha, beta, depth - 1);
        } else {
            score = -FN(negamax)(boardCopy, -beta, -alpha, depth - 1);
        }
#endif

        if (score > reference) {
            reference = score;
            *bestMove = moves[order[i]]; // Map back directly without fragile offset calculation
        }

        alpha = max(alpha, reference);
        if (alpha >= beta) break;
    }
#if SOLVER_USE_CACHE
    *solved = nodeSolved;
#endif
    return reference;
}

// --- Public: Distribution Root ---

#if SOLVER_USE_CACHE
void FN(distributionRoot)(Board *board, int *distribution, bool *solved, SolverConfig *config) {
#else
void FN(distributionRoot)(Board *board, int *distribution, bool *solvedOutput, SolverConfig *config) {
#endif
    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;

    int index = 5;
    int score;

#if SOLVER_USE_CACHE
    int depth = config->depth;
    if (config->depth == 0) {
        depth = MAX_DEPTH;
        setCacheMode(false, config->compressCache);
    } else {
        setCacheMode(true, config->compressCache);
    }
#else
    int depth = (config->depth == 0) ? MAX_DEPTH : config->depth;
#endif

    const int alpha = config->clip ? 0 : INT32_MIN + 1;
    const int beta  = config->clip ? 1 : INT32_MAX;

#if SOLVER_USE_CACHE
    bool nodeSolved = true;
#else
    solved = true;
#endif

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            distribution[index] = INT32_MIN;
            index--;
            continue;
        }
        Board boardCopy;
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

#if SOLVER_USE_CACHE
        bool childSolved;
        if (board->color == boardCopy.color) {
            score = FN(negamax)(&boardCopy, alpha, beta, depth, &childSolved);
        } else {
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;
#else
        if (board->color == boardCopy.color) {
            score = FN(negamax)(&boardCopy, alpha, beta, depth);
        } else {
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth);
        }
#endif

        if (config->clip && score > 1) score = 1;

        distribution[index] = score;
        index--;
    }

#if SOLVER_USE_CACHE
    *solved = nodeSolved;
    stepCache();
    resetCacheStats();
#else
    *solvedOutput = solved;
#endif
}

// --- Public: Aspiration Root ---

void FN(aspirationRoot)(Context* context, SolverConfig *config) {
    const int depthStep = 1;
    int currentDepth = 1;

#if SOLVER_USE_CACHE
    bool oneShot = false;

    // Check for No Iterative Deepening
    if (config->timeLimit == 0 && config->depth == 0) {
        currentDepth = MAX_DEPTH;
        oneShot = true;
        setCacheMode(false, config->compressCache);
    } else {
        setCacheMode(true, config->compressCache);
    }

    int bestMove = -1;
    int score = 0;
    bool solved = false;
#else
    int bestMove = -1;
    int score = 0;
#endif

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

    startProgress(config, PLAY_PREFIX);

    while (true) {
        solved = true;
        bool searchValid = false;

        if (config->clip) {
#if SOLVER_USE_CACHE
            // Clipped: Null Window (0, 1)
            score = FN(negamaxWithMove)(context->board, &bestMove, 0, 1, currentDepth, &solved);
#else
            // Null Window Search (0, 1)
            score = FN(negamaxWithMove)(context->board, &bestMove, 0, 1, currentDepth);
#endif
            searchValid = true;
        }
#if SOLVER_USE_CACHE
        else if (oneShot) {
            score = FN(negamaxWithMove)(context->board, &bestMove, INT32_MIN + 1, INT32_MAX, currentDepth, &solved);
            searchValid = true;
        }
#endif
        else {
#if SOLVER_USE_CACHE
            score = FN(negamaxWithMove)(context->board, &bestMove, alpha, beta, currentDepth, &solved);
#else
            score = FN(negamaxWithMove)(context->board, &bestMove, alpha, beta, currentDepth);
#endif

            if (score > alpha && score < beta) {
                searchValid = true;
                window = windowSize;

                alpha = score - window;
                beta  = score + window;
            } else {
                // Window Miss
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
            // Record Timing
#if SOLVER_USE_CACHE
            int timeIndex = oneShot ? 1 : currentDepth;
            if (depthTimes != NULL) {
                double t = (double)(clock() - start) / CLOCKS_PER_SEC;
                depthTimes[timeIndex] = t - lastTimeCaptured;
                lastTimeCaptured = t;
            }
#else
            if (depthTimes != NULL) {
                double t = (double)(clock() - start) / CLOCKS_PER_SEC;
                depthTimes[currentDepth] = t - lastTimeCaptured;
                lastTimeCaptured = t;
            }
#endif

            updateProgress(currentDepth, bestMove, score, nodeCount);

            // Check Exit Conditions
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
