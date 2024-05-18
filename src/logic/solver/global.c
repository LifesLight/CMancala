/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "logic/solver/global.h"

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

    return reference;
}

void GLOBAL_distributionRoot(Board *board, int *distribution, bool *solvedOutput, SolverConfig *config) {
    renderOutput("Thinking...", CHEAT_PREFIX);
    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;
    int index = 5;
    int score;
    int alpha = INT32_MIN + 1;
    int beta = INT32_MAX;

    if (config->goodEnough != 0) {
        beta = config->goodEnough;
        alpha = -config->goodEnough;
    }

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
            score = GLOBAL_negamax(&boardCopy, alpha, beta, config->depth);
        } else {
            score = -GLOBAL_negamax(&boardCopy, alpha, beta, config->depth);
        }

        if (config->goodEnough == 0) {
            alpha = max(alpha, score);
        }

        distribution[index] = score;
        index--;
    }

    *solvedOutput = solved;
}

void GLOBAL_aspirationRoot(Context* context, SolverConfig *config) {
    const int windowSize = 1;
    const int depthStep = 1;
    const int clip = config->goodEnough;

    if (clip != 0) {
        renderOutput("Clip is not yet supported in aspiration search!", PLAY_PREFIX);
    }

    int alpha = INT32_MIN + 1;
    int beta = INT32_MAX;
    int score;
    int currentDepth = 1;
    int bestMove = -1;
    int windowMisses = 0;
    clock_t start = clock();
    nodeCount = 0;

    renderOutput("Thinking...", PLAY_PREFIX);

    while (true) {
        solved = true;
        score = GLOBAL_negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth);
        if (score > alpha && score < beta) {
            currentDepth += depthStep;
            if (solved) break;
            if (currentDepth > config->depth && config->depth > 0) break;
            if (((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit && config->timeLimit > 0) break;
        } else {
            windowMisses++;
            alpha = score - windowSize;
            beta = score + windowSize;
        }
    }

    double timeTaken = (double)(clock() - start) / CLOCKS_PER_SEC;
    context->metadata.lastTime = timeTaken;
    context->metadata.lastNodes = nodeCount;
    if (windowMisses > currentDepth) {
        char message[256];
        snprintf(message, sizeof(message), "[WARNING]: High window misses! (You may increase \"windowSize\" in algo.c)");
        renderOutput(message, PLAY_PREFIX);
    }
    context->metadata.lastMove = bestMove;
    context->metadata.lastEvaluation = score;
    context->metadata.lastDepth = currentDepth - 1;
    context->metadata.lastSolved = solved;
}
