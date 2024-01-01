/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "algo.h"

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
    if (processBoardTerminal(board) || depth == 0) {
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

void negamaxRootHelper(Board *board, int *move, int *evaluation, int depthLimit, double timeLimit, bool useTimeLimit) {
    /**
     * Aspiration window search hyperparameters
    */
    const int aspirationWindowSize = 1;
    const int depthStep = 1;

    /**
     * Default values for alpha and beta
    */
    int alpha = INT32_MIN + 1;
    int beta = INT32_MAX;
    int previousScore = INT32_MIN;

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
    clock_t start = clock();

    /**
     * Iterative deepening loop
     * Runs until depth limit is reached or time limit is reached
    */
    while (true) {
        // Run negamax with move
        int score = negamaxWithMove(board, &bestMove, alpha, beta, currentDepth);

        printf("Window: [%d, %d], Score: %d, Depth: %d\n", alpha, beta, score, currentDepth);

        /**
         * Check if score is outside of aspiration window
         * If yes, offset window to include score
        */
        if (score > alpha && score < beta) {
            previousScore = score;
            currentDepth += depthStep;

            // Termination condition check, only if no window miss
            if ((!useTimeLimit && currentDepth > depthLimit) || 
                (useTimeLimit && (((double)(clock() - start) / CLOCKS_PER_SEC) >= timeLimit || currentDepth > depthLimit))) {
                break;
            }
        }

        previousScore = score;

        // Update alpha and beta
        alpha = previousScore - aspirationWindowSize;
        beta = previousScore + aspirationWindowSize;
    }

    if (useTimeLimit) {
        printf("Depth reached: %d\n", currentDepth - 1);
    }

    if (windowMisses > currentDepth / 4) {
        printf("[WARNING]: High window misses!\n");
    }

    *move = bestMove;
    *evaluation = previousScore;
}

void negamaxRootDepth(Board *board, int *move, int *evaluation, int depth) {
    negamaxRootHelper(board, move, evaluation, depth, 0, false);
}

void negamaxRootTime(Board *board, int *move, int *evaluation, double timeInSeconds) {
    // Using 100 depth as limit, since it is unlikely to be reached
    // Ever getting there strongly indicates the game is fully solved
    negamaxRootHelper(board, move, evaluation, 100, timeInSeconds, true);
}
