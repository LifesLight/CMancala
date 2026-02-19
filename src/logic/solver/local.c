/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/local.h"
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))



static inline int key(Board* board, int color1) {
    int res = color1 * getBoardEvaluation(board);
    if(color1 == board->color) res += 1000;
    return res;
}

static inline void sort(int *a, int n, Board* allBoards, int color1) {
    int keys[6];   // or MAX_MOVES

    for (int i = 0; i < n; i++) {
        keys[i] = key(&allBoards[a[i]], color1);
    }

    for (int i = 1; i < n; i++) {
        int x = a[i];
        int j = i - 1;


        while (j >= 0 && keys[j] < keys[i]) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = x;
    }
}


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
    int score;

    const int start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int end   = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;

    Board allMoves[6];
    int order[6];
    int keys[6];

    int valid = 0;

    for (int i = 0; i < 6; i++) {
        allMoves[i] = *board;

        if (board->cells[start - i] == 0) {
            continue;
        }
        order[valid++] = i;

        // Make copied board with move made
        makeMoveFunction(&allMoves[i], start - i);
        keys[i] = key(&allMoves[i], board->color);
    }

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

        if(best != i){
            int temp = order[i];
            order[i] = order[best];
            order[best] = temp;
        }


        bool childSolved;
        if (board->color == allMoves[order[i]].color) {
            score = LOCAL_negamax(&allMoves[order[i]], alpha, beta, depth - 1, &childSolved);
        } else {
            score = -LOCAL_negamax(&allMoves[order[i]], -beta, -alpha, depth - 1, &childSolved);
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
    *bestMove = -1;
    bool nodeSolved = true;

    const int start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    Board allMoves[6];
    int order[6];
    int keys[6];
    int valid = 0;

    for (int i = 0; i < 6; i++) {
        allMoves[i] = *board;

        if (board->cells[start - i] == 0) {
            continue;
        }
        order[valid++] = i;

        // Make copied board with move made
        makeMoveFunction(&allMoves[i], start - i);
        keys[i] = key(&allMoves[i], board->color);
    }

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

        if(best != i){
            int temp = order[i];
            order[i] = order[best];
            order[best] = temp;
        }


        // Make copied board with move made
        bool childSolved;
        if (board->color == allMoves[order[i]].color) {
            score = LOCAL_negamax(&allMoves[order[i]], alpha, beta, depth - 1, &childSolved);
        } else {
            score = -LOCAL_negamax(&allMoves[order[i]], -beta, -alpha, depth - 1, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;

        if (score > reference) {
            reference = score;
            *bestMove = start - order[i];
        }

        alpha = max(alpha, reference);
        if (alpha >= beta) break;
    }
    *solved = nodeSolved;
    return reference;
}

void LOCAL_distributionRoot(Board *board, int *distribution, bool *solved, SolverConfig *config) {
    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;
    int index = 5;
    int score;

    int depth = config->depth;
    if (config->depth == 0) {
        depth = MAX_DEPTH;
        setCacheMode(false, config->compressCache);
    } else {
        setCacheMode(true, config->compressCache);
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
    const int depthStep = 1;
    int currentDepth = 1;
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
            // Clipped: Null Window (0, 1)
            score = LOCAL_negamaxWithMove(context->board, &bestMove, 0, 1, currentDepth, &solved);
            searchValid = true;
        } else if (oneShot) {
            score = LOCAL_negamaxWithMove(context->board, &bestMove, INT32_MIN + 1, INT32_MAX, currentDepth, &solved);
            searchValid = true;
        } else {
            score = LOCAL_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth, &solved);

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

        stepCache();

        if (searchValid) {
            // Record Timing
            int timeIndex = oneShot ? 1 : currentDepth;
            if (depthTimes != NULL) {
                double t = (double)(clock() - start) / CLOCKS_PER_SEC;
                depthTimes[timeIndex] = t - lastTimeCaptured;
                lastTimeCaptured = t;
            }

            // Check Exit Conditions
            if (solved) break;
            if (oneShot) break;
            if (config->depth > 0 && currentDepth >= config->depth) break;
            if (config->timeLimit > 0 && ((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit) break;

            currentDepth += depthStep;
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
