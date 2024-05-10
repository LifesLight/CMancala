/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "logic/solver/quick.h"

bool solved;

int GLOBAL_negamax(Board *board, int alpha, const int beta, const int depth) {
    // Terminally check
    // The order of the checks is important here
    // Otherwise we could have a empty side, without adding up the opponents pieces to his score
    if (processBoardTerminal(board)) {
        return board->color * getBoardEvaluation(board);
    }

    // Check if depth limit is reached
    if (depth == 0) {
        // If we ever get depth limited in a non-terminal state, the game is not solved
        solved = false;
        return board->color * getBoardEvaluation(board);
    }

    nodeCount++;

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
        makeMoveFunction(&boardCopy, i);

        // Branch to check if this player is still playing
        if (board->color == boardCopy.color) {
            // If yes, call negamax with current parameters and no inversion
            score = GLOBAL_negamax(&boardCopy, alpha, beta, depth - 1);
        } else {
            // If no, call negamax with inverted parameters for the other player
            score = -GLOBAL_negamax(&boardCopy, -beta, -alpha, depth - 1);
        }

        // Update parameters
        reference = max(reference, score);
        alpha = max(alpha, reference);

        // If this branch certainly worse than another, prune it
        if (alpha >= beta) {
            break;
        }
    }

    // TODO: Do something if node is solved

    return reference;
}

int GLOBAL_negamaxWithMove(Board *board, int *bestMove, int alpha, const int beta, const int depth) {
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

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    Board boardCopy;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            continue;
        }
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

        if (alpha >= beta) {
            break;
        }
    }

    // TODO: Do something if node is solved

    return reference;
}

/**
 * Negamax with iterative deepening and aspiration window search
*/
void GLOBAL_negamaxAspirationRoot(Context* context) {
    INITIALIZE_VARS;
    ITERATIVE_DEEPENING_LOOP(
        GLOBAL_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth),
        if (solved) break;
    );
}

/**
 * Negamax root with distribution
 * Full search without any optimizations
*/
void GLOBAL_negamaxRootWithDistribution(Board *board, int depth, int32_t* distribution, bool *returnSolved) {
    solved = true;
    NEGAMAX_ROOT_BODY(board, depth, distribution, GLOBAL_negamax(&boardCopy, alpha, beta, depth - 1));
    *returnSolved = solved;
}
