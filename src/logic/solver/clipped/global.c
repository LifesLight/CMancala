/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "logic/solver/clipped/global.h"

bool solvedClipped;

int GLOBAL_CLIP_negamax(Board *board, const int depth) {
    // Terminally check
    // The order of the checks is important here
    // Otherwise we could have a empty side, without adding up the opponents pieces to his score
    if (processBoardTerminal(board)) {
        return board->color * getBoardEvaluation(board);
    }

    // Check if depth limit is reached
    if (depth == 0) {
        // If we ever get depth limited in a non-terminal state, the game is not solved
        solvedClipped = false;
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
            score = GLOBAL_CLIP_negamax(&boardCopy, depth - 1);
        } else {
            // If no, call negamax with inverted parameters for the other player
            score = -GLOBAL_CLIP_negamax(&boardCopy, depth - 1);
        }

        // Update parameters
        reference = max(reference, score);

        // If this branch certainly worse than another, prune it
        if (reference >= 1) {
            break;
        }
    }

    return reference;
}

int GLOBAL_CLIP_negamaxWithMove(Board *board, int *bestMove, const int depth) {
    if (processBoardTerminal(board)) {
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    if (depth == 0) {
        solvedClipped = false;
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
            score = GLOBAL_CLIP_negamax(&boardCopy, depth - 1);
        } else {
            score = -GLOBAL_CLIP_negamax(&boardCopy, depth - 1);
        }

        if (score > reference) {
            reference = score;
            *bestMove = i;
        }

        if (reference >= 1) {
            break;
        }
    }

    return reference;
}

void GLOBAL_CLIP_distributionRoot(Board *board, int *distribution, bool *solvedOutput, SolverConfig *config) {
    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;
    int index = 5;
    int score;

    solvedClipped = true;

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
            score = GLOBAL_CLIP_negamax(&boardCopy, config->depth);
        } else {
            score = -GLOBAL_CLIP_negamax(&boardCopy, config->depth);
        }

        distribution[index] = score;
        index--;
    }

    *solvedOutput = solvedClipped;
}

void GLOBAL_CLIP_aspirationRoot(Context* context, SolverConfig *config) {
    const int depthStep = 1;

    int score;
    int currentDepth = 1;
    int bestMove = -1;
    clock_t start = clock();
    nodeCount = 0;

    while (true) {
        solvedClipped = true;
        score = GLOBAL_CLIP_negamaxWithMove(context->board, &bestMove, currentDepth);
        currentDepth += depthStep;
        if (solvedClipped) break;
        if (currentDepth > config->depth && config->depth > 0) break;
        if (((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit && config->timeLimit > 0) break;
    }

    double timeTaken = (double)(clock() - start) / CLOCKS_PER_SEC;
    context->metadata.lastTime = timeTaken;
    context->metadata.lastNodes = nodeCount;
    context->metadata.lastMove = bestMove;
    context->metadata.lastEvaluation = score;
    context->metadata.lastDepth = currentDepth - 1;
    context->metadata.lastSolved = solvedClipped;

    if (score < 0) {
        renderOutput("[WARNING]: Clipped best move calculators should not be used in losing positions!", CHEAT_PREFIX);
    }
}
