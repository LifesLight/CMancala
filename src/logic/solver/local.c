/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "logic/solver/local.h"


int LOCAL_negamax(Board *board, int alpha, int beta, const int depth, bool* solved) {
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
            alpha = max(alpha, cachedValue);
        } else if (boundType == UPPER_BOUND) {
            beta = min(beta, cachedValue);
        }

        if (alpha >= beta) {
            *solved = true;
            return cachedValue;
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

    const int alphaOriginal = alpha;

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
            score = LOCAL_negamax(&boardCopy, alpha, beta, depth - 1, &solvedTemp);
        } else {
            // If no, call negamax with inverted parameters for the other player
            score = -LOCAL_negamax(&boardCopy, -beta, -alpha, depth - 1, &solvedTemp);
        }

        if (!solvedTemp) {
            *solved = false;
        }

        // Update parameters
        reference = max(reference, score);
        alpha = max(alpha, reference);

        // If this branch certainly worse than another, prune it
        if (alpha >= beta) {
            break;
        }
    }

    // If subtree is solved, cache it
    if (*solved) {
        if (reference <= alphaOriginal) {
            cacheNode(board, reference, UPPER_BOUND);
        } else if (reference >= beta) {
            cacheNode(board, reference, LOWER_BOUND);
        } else {
            cacheNode(board, reference, EXACT_BOUND);
        }
    }

    return reference;
}

int LOCAL_negamaxWithMove(Board *board, int *bestMove, int alpha, int beta, const int depth, bool* solved) {
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
            score = LOCAL_negamax(&boardCopy, alpha, beta, depth - 1, &solvedTemp);
        } else {
            score = -LOCAL_negamax(&boardCopy, -beta, -alpha, depth - 1, &solvedTemp);
        }

        if (!solvedTemp) {
            *solved = false;
        }

        if (score > reference) {
            reference = score;
            *bestMove = i;
        }
        alpha = max(alpha, reference);

        if (alpha >= beta) {
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

/**
 * Negamax with iterative deepening and aspiration window search
*/
void LOCAL_negamaxAspirationRoot(Context* context) {
    if (getCacheSize() == 0) {
        renderOutput("Cache automatically enabled with \"normal\" preset", PLAY_PREFIX);
        startCache(NORMAL_CACHE_SIZE);
    }

    INITIALIZE_VARS;
    bool solved;
    ITERATIVE_DEEPENING_LOOP(
        LOCAL_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth, &solved),
        if (solved) break;
    );

    stepCache();
}

/**
 * Negamax root with distribution
 * Full search without any optimizations
*/
void LOCAL_negamaxRootWithDistribution(Board *board, int depth, int32_t* distribution, bool* solved) {
    if (getCacheSize() == 0) {
        renderOutput("Cache automatically enabled with \"normal\" preset", CHEAT_PREFIX);
        startCache(NORMAL_CACHE_SIZE);
    }

    NEGAMAX_ROOT_BODY(board, depth, distribution, LOCAL_negamax(&boardCopy, alpha, beta, depth - 1, solved));

    stepCache();
}

/**
 * We can prob save the player bit by normalizing the layout :D
 * This should also give better performance
*/
