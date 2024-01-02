/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "algo.h"

bool solved = true;

int min(const int a, const int b) {
    return (a < b) ? a : b;
}

int max(const int a, const int b) {
    return (a > b) ? a : b;
}

int negamax(Board *board, int alpha, const int beta, const int depth) {
    // Terminally check
    // The order of the checks is important here
    // Otherwise we could have a empty side, without adding up the opponents pieces to his score
    if (processBoardTerminal(board)) {
        //printf("Terminal state reached!\n");
        return board->color * getBoardEvaluation(board);
    }

    // Check if depth limit is reached
    if (depth == 0) {
        // If we ever get depth limited in a non-terminal state, the game is not solved
        solved = false;
        return board->color * getBoardEvaluation(board);
    }

    // Keeping track of best score
    int reference = INT32_MIN;

    // Will be needed in every iteration
    Board boardCopy;
    int score;

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
        makeMoveOnBoard(&boardCopy, i);

        // Branch to check if this player is still playing
        if (board->color == boardCopy.color)
            // If yes, call negamax with current parameters and no inversion
            score = negamax(&boardCopy, alpha, beta, depth - 1);
        else
            // If no, call negamax with inverted parameters for the other player
            score = -negamax(&boardCopy, -beta, -alpha, depth - 1);

        // Update parameters
        reference = max(reference, score);
        alpha = max(alpha, reference);

        // If this branch certainly worse than another, prune it
        if (alpha >= beta) {
            break;
        }
    }

    return reference;
}

int negamaxWithMove(Board *board, int *bestMove, int alpha, const int beta, const int depth) {
    if (processBoardTerminal(board) || depth == 0) {
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    int reference = INT32_MIN;
    int score;

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    Board boardCopy;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            continue;
        }
        copyBoard(board, &boardCopy);
        makeMoveOnBoard(&boardCopy, i);

        if (board->color == boardCopy.color)
            score = negamax(&boardCopy, alpha, beta, depth - 1);
        else
            score = -negamax(&boardCopy, -beta, -alpha, depth - 1);

        if (score > reference) {
            reference = score;
            *bestMove = i;
        }

        alpha = max(alpha, reference);

        if (alpha >= beta) {
            break;
        }
    }

    return reference;
}

// Helper function containing shared logic
// Mainly implements aspiration window and depth / time limit
static void negamaxRootHelper(Board *board, int *move, int *evaluation, int depthLimit, double timeLimit, bool useTimeLimit) {
    /**
     * Aspiration window search hyperparameters
    */
    const int windowSize = 1;
    const int depthStep = 1;

    /**
     * Default values for alpha and beta
    */
    int alpha = INT32_MIN + 1;
    int beta = INT32_MAX;
    int score;

    /**
     * Iterative deepening runnning parameters
    */
    int currentDepth = 1;
    int bestMove;

    // Used to keep track of window misses for warning
    int windowMisses = 0;

    /**
     * Start timer for time limit
    */
    clock_t start;

    if (useTimeLimit) {
        start = clock();
    }

    /**
     * Iterative deepening loop
     * Runs until depth limit is reached or time limit is reached
    */
    while (true) {
        // Run negamax with move tracking

        // Track if game is solved
        // If game is still solved after search, we can stop
        solved = true;
        score = negamaxWithMove(board, &bestMove, alpha, beta, currentDepth);

        /**
         * Check if score is outside of aspiration window
         * If yes, research with offset window
        */

        // If score is inside window, increase depth
        if (score > alpha && score < beta) {
            currentDepth += depthStep;

            // Check if game is solved
            if (solved) {
                printf("Game is solved!\n");
                break;
            }

            // Termination condition check, only if no window miss
            if ((currentDepth > depthLimit) ||
                    (useTimeLimit && (((double)(clock() - start) / CLOCKS_PER_SEC) >= timeLimit))) {
                break;
            }

        // If score is outside window, research with offset window
        } else {
            windowMisses++;
        }

        // Update alpha and beta for new search
        alpha = score - windowSize;
        beta = score + windowSize;
    }

    // Inform about depth reached when using time limit
    if (useTimeLimit) {
        printf("Depth reached: %d\n", currentDepth - 1);
    }

    // Warn about high window misses
    // When this happens often, the window size should be increased
    if (windowMisses > currentDepth) {
        printf("[WARNING]: High window misses! (You may increase \"windowSize\" in algo.c)\n");
    }

    *move = bestMove;
    *evaluation = score;
}

void negamaxRootDepth(Board *board, int *move, int *evaluation, int depth) {
    negamaxRootHelper(board, move, evaluation, depth, 0, false);
}

void negamaxRootTime(Board *board, int *move, int *evaluation, double timeInSeconds) {
    // Using effectively infinite depth limit
    negamaxRootHelper(board, move, evaluation, INT32_MAX, timeInSeconds, true);
}
