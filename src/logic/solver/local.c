/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "logic/solver/local.h"


int LOCAL_negamax(Board *board, int alpha, const int beta, const int depth, bool* solvedSubtree) {
    // Terminally check
    // The order of the checks is important here
    // Otherwise we could have a empty side, without adding up the opponents pieces to his score
    if (processBoardTerminal(board)) {
        return board->color * getBoardEvaluation(board);
    }

    // Check if depth limit is reached
    if (depth == 0) {
        // If we ever get depth limited in a non-terminal state, the game is not solved
        *solvedSubtree = false;
        return board->color * getBoardEvaluation(board);
    }

    const int windowId = alpha + 1;

    // Check if board is cached
    int cachedValue = getCachedValue(board, windowId);
    if (cachedValue != INT32_MIN) {
        return cachedValue;
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
            score = LOCAL_negamax(&boardCopy, alpha, beta, depth - 1, solvedSubtree);
        } else {
            // If no, call negamax with inverted parameters for the other player
            score = -LOCAL_negamax(&boardCopy, -beta, -alpha, depth - 1, solvedSubtree);
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
    if (*solvedSubtree) {
        cacheNode(board, reference, windowId);
    }

    return reference;
}

int LOCAL_negamaxWithMove(Board *board, int *bestMove, int alpha, const int beta, const int depth, bool* solvedSubtree) {
    if (processBoardTerminal(board)) {
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    if (depth == 0) {
        *solvedSubtree = false;
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
            score = LOCAL_negamax(&boardCopy, alpha, beta, depth - 1, solvedSubtree);
        } else {
            score = -LOCAL_negamax(&boardCopy, -beta, -alpha, depth - 1, solvedSubtree);
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
void LOCAL_negamaxAspirationRoot(Context* context) {
    INITIALIZE_VARS;
    bool solved = true;
    ITERATIVE_DEEPENING_LOOP(
        LOCAL_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth, &solved),
        if (solved) break;
    );
}

/**
 * Negamax root with distribution
 * Full search without any optimizations
*/
void LOCAL_negamaxRootWithDistribution(Board *board, int depth, int32_t* distribution, bool* solved) {
    NEGAMAX_ROOT_BODY(board, depth, distribution, LOCAL_negamax(&boardCopy, alpha, beta, depth - 1, solved));
}
