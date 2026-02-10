/**
 * Copyright (c) Alexander Kurtz 2026
 */

#ifndef TOKEN_PASTE
#define TOKEN_PASTE(x, y) x ## _ ## y
#define CAT(x, y) TOKEN_PASTE(x, y)
#endif

#define FN(name) CAT(PREFIX, name)

int FN(negamax)(Board *board, int alpha, const int beta, const int depth) {
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
            score = FN(negamax)(&boardCopy, alpha, beta, depth - 1);
        } else {
            // If no, call negamax with inverted parameters for the other player
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth - 1);
        }

        // Update parameters
        reference = max(reference, score);
        alpha = max(alpha, reference);

        // If this branch certainly worse than another, prune it
#if IS_CLIPPED
        if (alpha >= beta || alpha >= 1) {
            break;
        }
#else
        if (alpha >= beta) {
            break;
        }
#endif

    }
    return reference;
}

#if IS_CLIPPED
int FN(negamaxWithMove)(Board *board, int *bestMove, const int depth) {
#else
int FN(negamaxWithMove)(Board *board, int *bestMove, int alpha, const int beta, const int depth) {
#endif

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

#if IS_CLIPPED
    const int alpha = INT32_MIN + 1;
    const int beta = INT32_MAX;
#endif

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            continue;
        }
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);

        if (board->color == boardCopy.color) {
            score = FN(negamax)(&boardCopy, alpha, beta, depth - 1);
        } else {

#if IS_CLIPPED
            score = -FN(negamax)(&boardCopy, alpha, beta, depth - 1);
#else
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth - 1);
#endif

        }
        if (score > reference) {
            reference = score;
            *bestMove = i;
        }

#if IS_CLIPPED
        if (reference >= 1) {
            break;
        }
#else
        alpha = max(alpha, reference);
        if (alpha >= beta) {
            break;
        }
#endif

    }
    return reference;
}
void FN(distributionRoot)(Board *board, int *distribution, bool *solvedOutput, SolverConfig *config) {
    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;

    int index = 5;
    int score;

    const int alpha = INT32_MIN + 1;
    const int beta = INT32_MAX;

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
            score = FN(negamax)(&boardCopy, alpha, beta, config->depth);
        } else {
            score = -FN(negamax)(&boardCopy, alpha, beta, config->depth);
        }

        distribution[index] = score;
        index--;
    }

    *solvedOutput = solved;
}

void FN(aspirationRoot)(Context* context, SolverConfig *config) {
    const int depthStep = 1;
    int currentDepth = 1;
    int bestMove = -1;
    int score = 0;

#if !IS_CLIPPED
    const int windowSize = 1;
    int window = windowSize;
    int alpha = INT32_MIN + 1;
    int beta  = INT32_MAX;
    int windowMisses = 0;
#endif

    clock_t start = clock();
    nodeCount = 0;

    double* depthTimes = context->metadata.lastDepthTimes;
    if (depthTimes != NULL) {
        for (int i = 0; i < MAX_DEPTH; i++) depthTimes[i] = -1.0;
    }
    double lastTimeCaptured = 0.0;

    while (true) {
        solved = true;

#if IS_CLIPPED
        score = FN(negamaxWithMove)(context->board, &bestMove, currentDepth);

        if (depthTimes != NULL && currentDepth < MAX_DEPTH) {
            double t = (double)(clock() - start) / CLOCKS_PER_SEC;
            depthTimes[currentDepth] = t - lastTimeCaptured;
            lastTimeCaptured = t;
        }

        currentDepth += depthStep;

        if (solved) break;
        if (config->depth > 0 && currentDepth > config->depth) break;
        if (config->timeLimit > 0 &&
            ((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit) break;

#else
        score = FN(negamaxWithMove)(context->board, &bestMove, alpha, beta, currentDepth);

        if (score > alpha && score < beta) {
            if (depthTimes != NULL && currentDepth < MAX_DEPTH) {
                double t = (double)(clock() - start) / CLOCKS_PER_SEC;
                depthTimes[currentDepth] = t - lastTimeCaptured;
                lastTimeCaptured = t;
            }

            currentDepth += depthStep;
            window = windowSize;

            if (solved) break;
            if (config->depth > 0 && currentDepth > config->depth) break;
            if (config->timeLimit > 0 &&
                ((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit) break;
        } else {
            windowMisses++;
            window *= 2;
            alpha = score - window;
            beta  = score + window;
        }
#endif
    }

    double timeTaken = (double)(clock() - start) / CLOCKS_PER_SEC;
    context->metadata.lastTime = timeTaken;
    context->metadata.lastNodes = nodeCount;
    context->metadata.lastMove = bestMove;
    context->metadata.lastEvaluation = score;
    context->metadata.lastDepth = currentDepth - 1;
    context->metadata.lastSolved = solved;

#if !IS_CLIPPED
    if (windowMisses > currentDepth) {
        char msg[256];
        snprintf(msg, sizeof(msg),
            "[WARNING]: High window misses! (You may increase \"windowSize\" in algo.c)");
        renderOutput(msg, PLAY_PREFIX);
    }
#else
    if (score < 0) {
        renderOutput(
            "[WARNING]: Clipped best move calculators should not be used in losing positions!",
            CHEAT_PREFIX
        );
    }
#endif
}

#undef FN
