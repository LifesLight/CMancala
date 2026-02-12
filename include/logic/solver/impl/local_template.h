/**
 * Copyright (c) Alexander Kurtz 2026
 */

#ifndef TOKEN_PASTE
#define TOKEN_PASTE(x, y) x ## _ ## y
#define CAT(x, y) TOKEN_PASTE(x, y)
#endif

#define FN(name) CAT(PREFIX, name)

int FN(negamax)(Board *board, int alpha, int beta, const int depth, bool *solved) {
    if (processBoardTerminal(board)) {
        *solved = true;
        return board->color * getBoardEvaluation(board);
    }

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

    if (depth == 0) {
        *solved = false;
        return board->color * getBoardEvaluation(board);
    }

    int reference = INT32_MIN;
    const int alphaOriginal = alpha;

    nodeCount++;
    bool nodeSolved = true;

    Board boardCopy;
    int score;

    const int8_t start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int8_t end   = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;

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

    int depth = config->depth;
    if (config->depth == 0) {
        depth = MAX_DEPTH;
    }

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
            score = FN(negamax)(&boardCopy, alpha, beta, depth, &childSolved);
        } else {
            score = -FN(negamax)(&boardCopy, alpha, beta, depth, &childSolved);
        }
        nodeSolved = nodeSolved && childSolved;
        distribution[index] = score;
        index--;
    }
    *solved = nodeSolved;
    stepCache();
    resetCacheStats();
}

void FN(aspirationRoot)(Context* context, SolverConfig *config) {
    if (getCacheSize() == 0) {
        renderOutput("Cache is disabled, starting with \"normal\" preset!", CHEAT_PREFIX);
        startCache(NORMAL_CACHE_SIZE);
    }

    const int depthStep = 1;
    int currentDepth = 1;
    bool oneShot = false;
    if (config->timeLimit == 0 && config->depth == 0) {
        currentDepth = MAX_DEPTH;
        oneShot = true;
        setCacheNoDepth(true);
    }

    int bestMove = -1;
    int score = 0;
    bool solved = false;

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
        score = FN(negamaxWithMove)(
            context->board,
            &bestMove,
            currentDepth,
            &solved
        );
#else
        score = FN(negamaxWithMove)(
            context->board,
            &bestMove,
            alpha,
            beta,
            currentDepth,
            &solved
        );
#endif

        int timeIndex = oneShot ? 1 : currentDepth;

        if (depthTimes != NULL) {
            double t = (double)(clock() - start) / CLOCKS_PER_SEC;
            depthTimes[timeIndex] = t - lastTimeCaptured;
            lastTimeCaptured = t;
        }

        stepCache();

        if (solved) break;
        if (config->depth > 0 && currentDepth >= config->depth) break;
        if (config->timeLimit > 0 &&
            ((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit) break;

        currentDepth += depthStep;

#if !IS_CLIPPED
        if (!oneShot) {
            if (score <= alpha || score >= beta) {
                windowMisses++;
                window *= 2;
                alpha = score - window;
                beta  = score + window;
                continue;
            }
            window = windowSize;
        }
#endif
    }

    context->metadata.lastTime =
        (double)(clock() - start) / CLOCKS_PER_SEC;
    context->metadata.lastNodes = nodeCount;
    context->metadata.lastMove = bestMove;
    context->metadata.lastEvaluation = score;
    context->metadata.lastDepth = currentDepth;
    context->metadata.lastSolved = solved;

#if !IS_CLIPPED
    if (windowMisses > currentDepth) {
        renderOutput(
            "[WARNING]: High window misses! (You may increase \"windowSize\")",
            PLAY_PREFIX
        );
    }
#else
    if (score < 0) {
        renderOutput(
            "[WARNING]: Clipped best move calculators should not be used in losing positions!",
            CHEAT_PREFIX
        );
    }
#endif

    resetCacheStats();
}

#undef FN
