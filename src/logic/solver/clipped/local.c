/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "logic/solver/local.h"


int LOCAL_CLIP_negamax(Board *board, int alpha, int beta, const int depth, bool* solved) {
    // Terminally check
    // The order of the checks is important here
    // Otherwise we could have a empty side, without adding up the opponents pieces to his score
    if (processBoardTerminal(board)) {
        *solved = true;
        return board->color * getBoardEvaluation(board);
    }

    // Check if depth limit is reached
    if (depth == 0) {
        // If we ever get depth limited in a non-terminal state, the game is not solved
        *solved = false;
        return board->color * getBoardEvaluation(board);
    }

    int reference = INT32_MIN;
    // Check if board is cached
    int cachedValue;
    int boundType;
    if (getCachedValue(board, &cachedValue, &boundType)) {
        if (boundType == EXACT_BOUND) {
            *solved = true;
            return cachedValue;
        } else if (boundType == LOWER_BOUND) {
            if (cachedValue >= beta) {
                *solved = true;
                return cachedValue;
            }
        }
    }

    nodeCount++;

    // Will be needed in every iteration
    Board boardCopy;
    int score;
    bool solvedTemp;
    *solved = true;

    // Iterate over all possible moves
    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    for (int8_t i = start; i >= end; i--) {
        // Filter invalid moves
        if (board->cells[i] == 0) {
            continue;
        }

        // Make copied board with move made
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        solvedTemp = false;

        // Branch to check if this player is still playing
        if (board->color == boardCopy.color) {
            // If yes, call negamax with current parameters and no inversion
            score = LOCAL_CLIP_negamax(&boardCopy, alpha, beta, depth - 1, &solvedTemp);
        } else {
            // If no, call negamax with inverted parameters for the other player
            score = -LOCAL_CLIP_negamax(&boardCopy, -beta, -alpha, depth - 1, &solvedTemp);
        }

        if (!solvedTemp) {
            *solved = false;
        }

        // Update parameters
        reference = max(reference, score);

        // If this branch certainly worse than another, prune it
        if (reference >= beta) {
            break;
        }
    }

    // If subtree is solved, cache it
    if (*solved) {
        if (reference <= alpha) {
            cacheNode(board, reference, UPPER_BOUND);
        } else if (reference >= beta) {
            cacheNode(board, reference, LOWER_BOUND);
        } else {
            cacheNode(board, reference, EXACT_BOUND);
        }
    }

    return reference;
}

int LOCAL_CLIP_negamaxWithMove(Board *board, int *bestMove, int alpha, int beta, const int depth, bool* solved) {
    if (processBoardTerminal(board)) {
        *bestMove = -1;
        *solved = true;
        return board->color * getBoardEvaluation(board);
    }

    if (depth == 0) {
        *solved = false;
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    // Can't cache here, since we need to know the best move

    nodeCount++;

    int reference = INT32_MIN;
    int score;
    bool solvedTemp;
    *solved = true;

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    Board boardCopy;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            continue;
        }
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        solvedTemp = false;

        if (board->color == boardCopy.color) {
            score = LOCAL_CLIP_negamax(&boardCopy, alpha, beta, depth - 1, &solvedTemp);
        } else {
            score = -LOCAL_CLIP_negamax(&boardCopy, -beta, -alpha, depth - 1, &solvedTemp);
        }

        if (!solvedTemp) {
            *solved = false;
        }

        if (score > reference) {
            reference = score;
            *bestMove = i;
        }

        if (reference >= beta) {
            break;
        }
    }

#ifndef DEBUG_CACHE
    // If solved, cache it
    if (*solved && alpha == reference) {
        cacheNode(board, reference);
    }
#endif

    return reference;
}

void LOCAL_CLIP_distributionRoot(Board *board, int *distribution, bool *solved, SolverConfig *config) {
    // Cache checking
    if (getCacheSize() == 0) {
        renderOutput("Cache is disabled, starting with \"normal\" preset!", CHEAT_PREFIX);
        startCache(NORMAL_CACHE_SIZE);
    }

    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;
    int index = 5;
    int score;

    int beta = config->goodEnough;
    int alpha = -config->goodEnough;

    bool solvedTemp;
    *solved = true;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            distribution[index] = INT32_MIN;
            index--;
            continue;
        }
        Board boardCopy;
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        solvedTemp = false;

        if (board->color == boardCopy.color) {
            score = LOCAL_CLIP_negamax(&boardCopy, alpha, beta, config->depth, &solvedTemp);
        } else {
            score = -LOCAL_CLIP_negamax(&boardCopy, alpha, beta, config->depth, &solvedTemp);
        }

        if (!solvedTemp) {
            *solved = false;
        }

        distribution[index] = score;
        index--;
    }

    stepCache();
}

void LOCAL_CLIP_aspirationRoot(Context* context, SolverConfig *config) {
    // Cache checking
    if (getCacheSize() == 0) {
        renderOutput("Cache is disabled, starting with \"normal\" preset!", CHEAT_PREFIX);
        startCache(NORMAL_CACHE_SIZE);
    }

    const int depthStep = 1;
    const int clip = config->goodEnough;

    int alpha = -clip;
    int beta = clip;
    int score;
    int currentDepth = 1;
    int bestMove = -1;
    clock_t start = clock();
    nodeCount = 0;
    bool solved;

    while (true) {
        solved = false;
        score = LOCAL_CLIP_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth, &solved);
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

    stepCache();
}
