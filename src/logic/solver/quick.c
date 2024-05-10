/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "logic/solver/quick.h"

int goodEnough = 1;

int QUICK_negamax(Board *board, int alpha, const int beta, const int depth, bool* solved) {
    // Terminally check
    // The order of the checks is important here
    // Otherwise we could have a empty side, without adding up the opponents pieces to his score
    if (processBoardTerminal(board)) {
        return board->color * getBoardEvaluation(board);
    }

    // Check if depth limit is reached
    if (depth == 0) {
        // If we ever get depth limited in a non-terminal state, the game is not solved
        *solved = false;
        return board->color * getBoardEvaluation(board);
    }

    nodeCount++;

    // Keeping track of best score
    int reference = INT32_MIN;

    // Will be needed in every iteration
    Board boardCopy;
    int score;
    bool solvedTemp;

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
        solvedTemp = true;

        // Branch to check if this player is still playing
        if (board->color == boardCopy.color) {
            // If yes, call negamax with current parameters and no inversion
            score = QUICK_negamax(&boardCopy, alpha, beta, depth - 1, &solvedTemp);
        } else {
            // If no, call negamax with inverted parameters for the other player
            score = -QUICK_negamax(&boardCopy, -beta, -alpha, depth - 1, &solvedTemp);
        }

        // Update parameters
        if (score > reference) {
            reference = score;
            if (solvedTemp) {
                *solved = true;

                if (score >= goodEnough) {
                    break;
                }
            } else {
                *solved = false;
            }
        }
        alpha = max(alpha, reference);

        // If this branch certainly worse than another, prune it
        if (alpha >= beta) {
            break;
        }
    }

    // TODO: Do something if node is solved

    return reference;
}

int QUICK_negamaxWithMove(Board *board, int *bestMove, int alpha, const int beta, const int depth, bool* solved) {
    if (processBoardTerminal(board)) {
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    if (depth == 0) {
        *solved = false;
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    nodeCount++;

    int reference = INT32_MIN;
    int score;
    bool solvedTemp;

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    Board boardCopy;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            continue;
        }
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);
        solvedTemp = true;

        if (board->color == boardCopy.color) {
            score = QUICK_negamax(&boardCopy, alpha, beta, depth - 1, &solvedTemp);
        } else {
            score = -QUICK_negamax(&boardCopy, -beta, -alpha, depth - 1, &solvedTemp);
        }

        if (score > reference) {
            reference = score;
            *bestMove = i;
            if (solvedTemp) {
                *solved = true;

                if (score >= goodEnough) {
                    break;
                }
            } else {
                *solved = false;
            }
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
void QUICK_negamaxAspirationRoot(Context* context) {
    INITIALIZE_VARS;
    bool solved = true;
    ITERATIVE_DEEPENING_LOOP(
        QUICK_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth, &solved),
        if (solved && score >= goodEnough) break;
    );
}

/**
 * Negamax root with distribution
 * Full search without any optimizations
*/
void QUICK_negamaxRootWithDistribution(Board *board, int depth, int32_t* distribution, bool* solved) {
    NEGAMAX_ROOT_BODY(board, depth, distribution, QUICK_negamax(&boardCopy, alpha, beta, depth - 1, solved));
}

void QUICK_setGoodEnough(int value) {
    goodEnough = value;
}
