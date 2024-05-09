/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "algo.h"

#ifdef TRACK_THROUGHPUT
int64_t nodes;
#endif


#ifndef _WIN32
int min(const int a, const int b) {
    return (a < b) ? a : b;
}

int max(const int a, const int b) {
    return (a > b) ? a : b;
}
#endif

int negamax(Board *board, int alpha, const int beta, const int depth, bool* solved) {
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

    #ifdef TRACK_THROUGHPUT
    nodes++;
    #endif

    // Keeping track of best score
    int reference = INT32_MIN;

    // Will be needed in every iteration
    Board boardCopy;
    int score;

    #ifdef GREEDY_SOLVING
    bool solvedTemp;
    #endif

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

        #ifdef GREEDY_SOLVING
        solvedTemp = true;
        #endif

        // Branch to check if this player is still playing
        if (board->color == boardCopy.color) {
            // If yes, call negamax with current parameters and no inversion
            #ifdef GREEDY_SOLVING
            score = negamax(&boardCopy, alpha, beta, depth - 1, &solvedTemp);
            #else
            score = negamax(&boardCopy, alpha, beta, depth - 1, solved);
            #endif
        } else {
            // If no, call negamax with inverted parameters for the other player
            #ifdef GREEDY_SOLVING
            score = -negamax(&boardCopy, -beta, -alpha, depth - 1, &solvedTemp);
            #else
            score = -negamax(&boardCopy, -beta, -alpha, depth - 1, solved);
            #endif
        }

        // Update parameters
        #ifdef GREEDY_SOLVING
        if (score > reference) {
            reference = score;
            if (solvedTemp) {
                *solved = true;

                if (score >= GOOD_ENOUGH) {
                    break;
                }
            } else {
                *solved = false;
            }
        }
        #else
        reference = max(reference, score);
        #endif
        alpha = max(alpha, reference);

        // If this branch certainly worse than another, prune it
        if (alpha >= beta) {
            break;
        }
    }

    // TODO: Do something if node is solved

    return reference;
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
            tempResult =  negamaxWithTrace(&boardCopy, alpha, beta, depth - 1);
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

int negamaxWithMove(Board *board, int *bestMove, int alpha, const int beta, const int depth, bool* solved) {
    if (processBoardTerminal(board)) {
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    if (depth == 0) {
        *solved = false;
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    #ifdef TRACK_THROUGHPUT
    nodes++;
    #endif

    int reference = INT32_MIN;
    int score;

    #ifdef GREEDY_SOLVING
    bool solvedTemp;
    #endif

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    Board boardCopy;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            continue;
        }
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        #ifdef GREEDY_SOLVING
        solvedTemp = true;
        #endif

        if (board->color == boardCopy.color) {
            #ifdef GREEDY_SOLVING
            score = negamax(&boardCopy, alpha, beta, depth - 1, &solvedTemp);
            #else
            score = negamax(&boardCopy, alpha, beta, depth - 1, solved);
            #endif
        } else {
            #ifdef GREEDY_SOLVING
            score = -negamax(&boardCopy, -beta, -alpha, depth - 1, &solvedTemp);
            #else
            score = -negamax(&boardCopy, -beta, -alpha, depth - 1, solved);
            #endif
        }

        if (score > reference) {
            reference = score;
            *bestMove = i;
            #ifdef GREEDY_SOLVING
            if (solvedTemp) {
                *solved = true;

                if (score >= GOOD_ENOUGH) {
                    break;
                }
            } else {
                *solved = false;
            }
            #endif
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
void negamaxAspirationRoot(Context* context) {
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
     * Iterative deepening running parameters
    */
    int currentDepth = 1;
    int bestMove = -1;

    // Used to keep track of window misses for warning
    int windowMisses = 0;

    /**
     * Start timer for time limit
    */
    clock_t start = clock();

    /**
     * If tracking is enabled, reset nodes counter
    */
    #ifdef TRACK_THROUGHPUT
    nodes = 0;
    #endif

    renderOutput("Thinking...", PLAY_PREFIX);

    /**
     * Iterative deepening loop
     * Runs until depth limit is reached or time limit is reached
    */
    bool solved;
    while (true) {
        // Track if game is solved
        solved = true;
        score = negamaxWithMove(context->board, &bestMove, alpha, beta, currentDepth, &solved);

        /**
         * Check if score is outside of aspiration window
         * If yes, research with offset window
        */

        // If score is inside window, increase depth
        if (score > alpha && score < beta) {
            currentDepth += depthStep;

            // Check if game is solved
            #ifdef GREEDY_SOLVING
            if (solved && score >= GOOD_ENOUGH) {
                break;
            }
            #else
            if (solved) {
                break;
            }
            #endif

            // Check for depth limit
            if (currentDepth > context->config->depth && context->config->depth > 0) {
                break;
            }

            // Check for time limit
            if (((double)(clock() - start) / CLOCKS_PER_SEC) >= context->config->timeLimit && context->config->timeLimit > 0) {
                break;
            }
        } else {
            // If score is outside window, research with offset window
            windowMisses++;

            // Update alpha and beta for new search
            alpha = score - windowSize;
            beta = score + windowSize;
        }
    }

    /**
     * Compute real time taken
    */
    #ifdef TRACK_THROUGHPUT
    double timeTaken = (double)(clock() - start) / CLOCKS_PER_SEC;
    context->lastTime = timeTaken;
    context->lastNodes = nodes;
    #endif


    // Warn about high window misses
    // When this happens often, the window size should be increased
    if (windowMisses > currentDepth) {
        char message[256];
        snprintf(message, sizeof(message), "[WARNING]: High window misses! (You may increase \"windowSize\" in algo.c)");
        renderOutput(message, PLAY_PREFIX);
    }

    // Return best move and score
    context->lastMove = bestMove;
    context->lastEvaluation = score;
    context->lastDepth = currentDepth - 1;
    context->lastSolved = solved;
}

/**
 * Negamax root with distribution
 * Full search without any optimizations
*/
void negamaxRootWithDistribution(Board *board, int depth, int32_t* distribution, bool* solved) {
    int score;

    renderOutput("Thinking...", PLAY_PREFIX);

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    int index = 5;
    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            distribution[index] = INT32_MIN;
            index--;
            continue;
        }

        Board boardCopy;
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        int alpha = INT32_MIN + 1;
        int beta = INT32_MAX;

        if (board->color == boardCopy.color) {
            score = negamax(&boardCopy, alpha, beta, depth - 1, solved);
        } else {
            score = -negamax(&boardCopy, -beta, -alpha, depth - 1, solved);
        }

        distribution[index] = score;
        index--;
    }
}
