/**
 * Copyright (c) Alexander Kurtz 2026
 */

#ifndef TOKEN_PASTE
#define TOKEN_PASTE(x, y) x ## _ ## y
#define CAT(x, y) TOKEN_PASTE(x, y)
#endif

#define FN(name) CAT(PREFIX, name)

int FN(negamax)(Board *board, int alpha, int beta, const int depth) {
    // Terminally check
    // The order of the checks is important here
    // Otherwise we could have a empty side, without adding up the opponents pieces to his score
    if (processBoardTerminal(board)) {
        return board->color * getBoardEvaluation(board);
    }

    // Check if depth limit is reached
    if (depth == 0) {
        // If we ever get depth limited in a non-terminal state, the game is not solved
        return board->color * getBoardEvaluation(board);
    }

    int reference = INT32_MIN;
    // Check if board is cached
    int cachedValue;
    int boundType;
    if (getCachedValue(board, &cachedValue, &boundType)) {
        if (boundType == EXACT_BOUND) {
            return cachedValue;
        } else if (boundType == LOWER_BOUND) {
            alpha = max(alpha, cachedValue);
        } else if (boundType == UPPER_BOUND) {
            beta = min(beta, cachedValue);
        }

        if (alpha >= beta) {
            return cachedValue;
        }
    }

    nodeCount++;

    // Will be needed in every iteration
    Board boardCopy;
    int score;

    // Iterate over all possible moves
    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    const int alphaOriginal = alpha;

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

        // If we are somehow winning or alpha beta prune

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

    // TODO: Attempt Cache

    return reference;
}

#if IS_CLIPPED
int FN(negamaxWithMove)(Board *board, int *bestMove, const int depth) {
#else
int FN(negamaxWithMove)(Board *board, int *bestMove, int alpha, int beta, const int depth) {
#endif

    if (processBoardTerminal(board)) {
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    if (depth == 0) {
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    // Can't cache here, since we need to know the best move

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

void FN(distributionRoot)(Board *board, int *distribution, SolverConfig *config) {
    // Cache checking
    if (getCacheSize() == 0) {
        renderOutput("Cache is disabled, starting with \"normal\" preset!", CHEAT_PREFIX);
        startCache(NORMAL_CACHE_SIZE);
    }

    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;
    int index = 5;
    int score;

    const int alpha = INT32_MIN + 1;
    const int beta = INT32_MAX;

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

    stepCache();
}

#undef FN
