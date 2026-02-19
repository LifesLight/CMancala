/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/global.h"
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

static bool solved;

static int GLOBAL_negamax(Board *board, int alpha, int beta, const int depth) {
    if (processBoardTerminal(board)) {
        return board->color * getBoardEvaluation(board);
    }
    if (depth == 0) {
        solved = false;
        return board->color * getBoardEvaluation(board);
    }

    nodeCount++;
    int reference = INT32_MIN;
    int score;
    Board boardCopy;

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) continue;

        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        if (board->color == boardCopy.color) {
            score = GLOBAL_negamax(&boardCopy, alpha, beta, depth - 1);
        } else {
            score = -GLOBAL_negamax(&boardCopy, -beta, -alpha, depth - 1);
        }

        reference = max(reference, score);
        alpha = max(alpha, reference);

        if (alpha >= beta) break;
    }
    return reference;
}

int GLOBAL_negamaxWithMove(Board *board, int *bestMove, int alpha, int beta, const int depth) {
    if (processBoardTerminal(board)) {
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }
    if (depth == 0) {
        solved = false;
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    nodeCount++;
    int reference = INT32_MIN;
    int score;
    Board boardCopy;

    *bestMove = -1;

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) continue;

        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        if (board->color == boardCopy.color) {
            score = GLOBAL_negamax(&boardCopy, alpha, beta, depth - 1);
        } else {
            score = -GLOBAL_negamax(&boardCopy, -beta, -alpha, depth - 1);
        }

        if (score > reference) {
            reference = score;
            *bestMove = i;
        }

        alpha = max(alpha, reference);
        if (alpha >= beta) break;
    }
    return reference;
}

void GLOBAL_distributionRoot(Board *board, int *distribution, bool *solvedOutput, SolverConfig *config) {
    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;

    int index = 5;
    int score;
    int depth = (config->depth == 0) ? MAX_DEPTH : config->depth;

    const int alpha = config->clip ? 0 : INT32_MIN + 1;
    const int beta  = config->clip ? 1 : INT32_MAX;

    solved = true;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            distribution[index] = INT32_MIN;
            index--;
            continue;
        }
        Board boardCopy;
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        if (board->color == boardCopy.color) {
            score = GLOBAL_negamax(&boardCopy, alpha, beta, depth);
        } else {
            score = -GLOBAL_negamax(&boardCopy, -beta, -alpha, depth);
        }

        if (config->clip && score > 1) score = 1;

        distribution[index] = score;
        index--;
    }

    *solvedOutput = solved;
}

void GLOBAL_aspirationRoot(Context* context, SolverConfig *config) {
    const int depthStep = 1;
    int currentDepth = 1;
    int bestMove = -1;
    int score = 0;

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
            // Null Window Search (0, 1)
            score = GLOBAL_negamaxWithMove(context->board, &bestMove, 0, 1, currentDepth);
            searchValid = true; 
        } else {
            score = GLOBAL_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth);

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

        if (searchValid) {
            // Record Timing
            if (depthTimes != NULL) {
                double t = (double)(clock() - start) / CLOCKS_PER_SEC;
                depthTimes[currentDepth] = t - lastTimeCaptured;
                lastTimeCaptured = t;
            }

            // Check Exit Conditions
            if (solved) break;
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
}
