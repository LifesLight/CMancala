/**
 * Copyright (c) Alexander Kurtz 2026
 */


#include "logic/algo.h"


void aspirationRoot(Context* context, SolverConfig *config) {
    if (config->clip) {
        switch (config->solver) {
            case GLOBAL_SOLVER:
                GLOBAL_CLIP_aspirationRoot(context, config);
                break;
            case LOCAL_SOLVER:
                LOCAL_CLIP_aspirationRoot(context, config);
                break;
        }
    } else {
        switch (config->solver) {
            case GLOBAL_SOLVER:
                GLOBAL_aspirationRoot(context, config);
                break;
            case LOCAL_SOLVER:
                LOCAL_aspirationRoot(context, config);
                break;
        }
    }
}

void distributionRoot(Board *board, int32_t* distribution, bool *solved, SolverConfig *config) {
    if (config->clip) {
        switch (config->solver) {
            case GLOBAL_SOLVER:
                GLOBAL_CLIP_distributionRoot(board, distribution, solved, config);
                break;
            case LOCAL_SOLVER:
                LOCAL_CLIP_distributionRoot(board, distribution, config);
                // TODO: Reimplement solved
                *solved = false;
                break;
        }
    } else {
        switch (config->solver) {
            case GLOBAL_SOLVER:
                GLOBAL_distributionRoot(board, distribution, solved, config);
                break;
            case LOCAL_SOLVER:
                LOCAL_distributionRoot(board, distribution, config);
               // TODO: Reimplement solved
                *solved = false;
                break;
        }
    }
}

NegamaxTrace negamaxWithTrace(Board *board, int alpha, const int beta, const int depth) {
    NegamaxTrace result;
    result.score = INT32_MIN;
    result.moves = malloc(sizeof(int8_t) * (depth + 1));
    memset(result.moves, -1, sizeof(int8_t) * (depth + 1));

    if (processBoardTerminal(board)) {
        result.score = board->color * getBoardEvaluation(board);
        return result;
    }

    // Check if depth limit is reached
    if (depth == 0) {
        result.score = board->color * getBoardEvaluation(board);
        return result;
    }

    // Will be needed in every iteration
    Board boardCopy;
    NegamaxTrace tempResult;

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
            tempResult = negamaxWithTrace(&boardCopy, alpha, beta, depth - 1);
        } else {
            // If no, call negamax with inverted parameters for the other player
            tempResult = negamaxWithTrace(&boardCopy, -beta, -alpha, depth - 1);
            tempResult.score = -tempResult.score;
        }

        if (tempResult.score > result.score) {
            result.score = tempResult.score;
            memcpy(result.moves, tempResult.moves, sizeof(int8_t) * depth);
            result.moves[depth - 1] = i;
        }

        free(tempResult.moves);

        // Update parameters
        alpha = max(alpha, result.score);

        // If this branch certainly worse than another, prune it
        if (alpha >= beta) {
            break;
        }
    }

    return result;
}

NegamaxTrace traceRoot(Board *board, int alpha, const int beta, const int depth) {
    NegamaxTrace result;
    result.score = INT32_MIN;
    result.moves = malloc(sizeof(int8_t) * (depth + 1));
    memset(result.moves, -1, sizeof(int8_t) * (depth + 1));

    if (processBoardTerminal(board)) {
        result.score = board->color * getBoardEvaluation(board);
        return result;
    }

    // Check if depth limit is reached
    if (depth == 0) {
        result.score = board->color * getBoardEvaluation(board);
        return result;
    }

    // Will be needed in every iteration
    Board boardCopy;
    NegamaxTrace tempResult;

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
            tempResult = negamaxWithTrace(&boardCopy, alpha, beta, depth - 1);
        } else {
            // If no, call negamax with inverted parameters for the other player
            tempResult = negamaxWithTrace(&boardCopy, -beta, -alpha, depth - 1);
            tempResult.score = -tempResult.score;
        }

        if (tempResult.score > result.score) {
            result.score = tempResult.score;
            memcpy(result.moves, tempResult.moves, sizeof(int8_t) * depth);
            result.moves[depth - 1] = i;
        }

        free(tempResult.moves);

        // Update parameters
        alpha = max(alpha, result.score);

        // If this branch certainly worse than another, prune it
        if (alpha >= beta) {
            break;
        }
    }

    return result;
}
