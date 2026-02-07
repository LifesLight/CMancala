/**
 * Copyright (c) Alexander Kurtz 2026
 */

#ifndef TOKEN_PASTE
#define TOKEN_PASTE(x, y) x ## _ ## y
#define CAT(x, y) TOKEN_PASTE(x, y)
#endif

#define FN(name) CAT(PREFIX, name)

int FN(negamax)(Board *board, int alpha, int beta, const int depth, bool *solved) {
    // Terminally check
    // The order of the checks is important here
    // Otherwise we could have a empty side, without adding up the opponents pieces to his score
    if (processBoardTerminal(board)) {
        *solved = true;
        return board->color * getBoardEvaluation(board);
    }

    // Check if board is cached
    int cachedValue;
    int boundType;
    bool cachedSolved;

    if (getCachedValue(board, depth, &cachedValue, &boundType, &cachedSolved)) {
        if (boundType == EXACT_BOUND) {
            *solved = cachedSolved;
            return cachedValue;
        }

        if (boundType == LOWER_BOUND) {
            alpha = max(alpha, cachedValue);
        } else if (boundType == UPPER_BOUND) {
            beta = min(beta, cachedValue);
        }

        if (alpha >= beta) {
            *solved = cachedSolved;
            return cachedValue;
        }
    }

    // Check if depth limit is reached
    if (depth == 0) {
        // If we ever get depth limited in a non-terminal state, the game is not solved
        *solved = false;
        return board->color * getBoardEvaluation(board);
    }

    int reference = INT32_MIN;
    const int alphaOriginal = alpha;

    nodeCount++;
    bool nodeSolved = true;
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

        bool childSolved;
        // Branch to check if this player is still playing
        if (board->color == boardCopy.color) {
            // If yes, call negamax with current parameters and no inversion
            score = FN(negamax)(&boardCopy, alpha, beta, depth - 1, &childSolved);
        } else {
            // If no, call negamax with inverted parameters for the other player
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth - 1, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;
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

    // Cache node
    if (reference <= alphaOriginal) {
        boundType = UPPER_BOUND;
    } else if (reference >= beta) {
        boundType = LOWER_BOUND;
    } else {
        boundType = EXACT_BOUND;
    }

    cacheNode(board, reference, boundType, depth, nodeSolved);
    *solved = nodeSolved;
    return reference;
}

#if IS_CLIPPED
int FN(negamaxWithMove)(Board *board, int *bestMove, const int depth, bool *solved) {
#else
int FN(negamaxWithMove)(Board *board, int *bestMove, int alpha, int beta, const int depth, bool *solved) {
#endif

    if (processBoardTerminal(board)) {
        *bestMove = -1;
        *solved = true;
        return board->color * getBoardEvaluation(board);
    }

    if (depth == 0) {
        *bestMove = -1;
        *solved = false;
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
    bool nodeSolved = true;
    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            continue;
        }
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);
        bool childSolved;
        if (board->color == boardCopy.color) {
            score = FN(negamax)(&boardCopy, alpha, beta, depth - 1, &childSolved);
        } else {

#if IS_CLIPPED
            score = -FN(negamax)(&boardCopy, alpha, beta, depth - 1, &childSolved);
#else
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth - 1, &childSolved);
#endif

        }
        nodeSolved = nodeSolved && childSolved;
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

    *solved = nodeSolved;
    return reference;
}

void FN(distributionRoot)(Board *board, int *distribution, bool *solved, SolverConfig *config) {
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
    bool nodeSolved = true;

    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            distribution[index] = INT32_MIN;
            index--;
            continue;
        }
        Board boardCopy;
        copyBoard(board, &boardCopy);
        makeMoveFunction(&boardCopy, i);
        bool childSolved;
        if (board->color == boardCopy.color) {
            score = FN(negamax)(&boardCopy, alpha, beta, config->depth, &childSolved);
        } else {
            score = -FN(negamax)(&boardCopy, alpha, beta, config->depth, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;
        distribution[index] = score;
        index--;
    }
    *solved = nodeSolved;
    stepCache();
}

#undef FN
