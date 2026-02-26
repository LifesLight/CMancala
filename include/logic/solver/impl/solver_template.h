/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/impl/macros.h"
#include "logic/utility.h"
#include <pthread.h>
#include <stdatomic.h>

extern _Thread_local uint64_t nodeCount;
extern _Atomic uint64_t globalNodeCount;
extern _Atomic bool solver_abort_search;

#if !SOLVER_USE_CACHE
static _Thread_local bool solved;
#endif

static inline int FN(key)(Board* board, int color1) {
    if(color1 == board->color) return 1000;
    return color1 * getBoardEvaluation(board);
}

#if SOLVER_USE_CACHE
static int FN(negamax)(Board *board, int alpha, int beta, const int depth, bool *solved, int thread_id) {
#else
static int FN(negamax)(Board *board, int alpha, int beta, const int depth, int thread_id) {
#endif
    if (atomic_load_explicit(&solver_abort_search, memory_order_relaxed)) {
#if SOLVER_USE_CACHE
        *solved = false;
#endif
        return 0;
    }

    if (processBoardTerminal(board)) {
#if SOLVER_USE_CACHE
        *solved = true;
#endif
        return board->color * getBoardEvaluation(board);
    }

    nodeCount++;

#if SOLVER_USE_CACHE
    uint64_t boardHash = 0;
    bool hashValid = translateBoard(board, &boardHash);

    int cachedValue;
    int boundType;
    bool cachedSolved;
    if (hashValid && getCachedValueHash(board, boardHash, depth, &cachedValue, &boundType, &cachedSolved)) {
        if (boundType == EXACT_BOUND) {
            *solved = cachedSolved;
            return cachedValue;
        }
        if (boundType == LOWER_BOUND) alpha = max(alpha, cachedValue);
        else if (boundType == UPPER_BOUND) beta = min(beta, cachedValue);

        if (alpha >= beta) {
            *solved = cachedSolved;
            return cachedValue;
        }
    }
#endif

    if (depth == 0) {
#if SOLVER_USE_CACHE
        *solved = false;
#else
        solved = false;
#endif
        return board->color * getBoardEvaluation(board);
    }

    int reference = INT32_MIN;
    int score;

#if SOLVER_USE_CACHE
    const int alphaOriginal = alpha;
    bool nodeSolved = true;
#endif

    const int start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int end   = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;

    Board allMoves[6];
    int keys[6];
    int valid = 0;

    for (int i = start; i >= end; i--) {
        if (board->cells[i] == 0) continue;

        Board newBoard = *board;
        MAKE_MOVE(&newBoard, i);

        int k = FN(key)(&newBoard, board->color);

        int j = valid;
        while (j > 0 && keys[j - 1] < k) {
            keys[j] = keys[j - 1];
            allMoves[j] = allMoves[j - 1];
            j--;
        }
        keys[j] = k;
        allMoves[j] = newBoard;
        valid++;
    }

    for (int i = 0; i < valid; i++) {
        // PV-Preserving Shift: Thread 0 does 0, 1, 2. Thread 1 does 0, 2, 1.
        int idx = i;
        if (i > 0 && valid > 2 && thread_id > 0) {
            idx = 1 + ((i - 1 + thread_id) % (valid - 1));
        }

        Board *boardCopy = &allMoves[idx];

#if SOLVER_USE_CACHE
        bool childSolved;
        if (board->color == boardCopy->color) {
            score = FN(negamax)(boardCopy, alpha, beta, depth - 1, &childSolved, thread_id);
        } else {
            score = -FN(negamax)(boardCopy, -beta, -alpha, depth - 1, &childSolved, thread_id);
        }
        nodeSolved = nodeSolved && childSolved;
#else
        if (board->color == boardCopy->color) {
            score = FN(negamax)(boardCopy, alpha, beta, depth - 1, thread_id);
        } else {
            score = -FN(negamax)(boardCopy, -beta, -alpha, depth - 1, thread_id);
        }
#endif

        reference = max(reference, score);
        alpha = max(alpha, reference);

        if (alpha >= beta) break;
    }

#if SOLVER_USE_CACHE
    if (hashValid) {
        if (reference <= alphaOriginal) boundType = UPPER_BOUND;
        else if (reference >= beta) boundType = LOWER_BOUND;
        else boundType = EXACT_BOUND;

        cacheNodeHash(board, boardHash, reference, boundType, depth, nodeSolved);
    }
    *solved = nodeSolved;
#endif
    return reference;
}

#if SOLVER_USE_CACHE
int FN(negamaxWithMove)(Board *board, int *bestMove, int alpha, int beta, const int depth, bool *solved, int previousBestMove, int thread_id) {
#else
int FN(negamaxWithMove)(Board *board, int *bestMove, int alpha, int beta, const int depth, int previousBestMove, int thread_id) {
#endif
    if (atomic_load_explicit(&solver_abort_search, memory_order_relaxed)) {
        *bestMove = -1;
#if SOLVER_USE_CACHE
        *solved = false;
#endif
        return 0;
    }

    if (processBoardTerminal(board)) {
#if SOLVER_USE_CACHE
        *solved = true;
#endif
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }
    if (depth == 0) {
        *bestMove = -1;
#if SOLVER_USE_CACHE
        *solved = false;
#else
        solved = false;
#endif
        return board->color * getBoardEvaluation(board);
    }

    nodeCount++;

    int reference = INT32_MIN;
    int score;
    *bestMove = -1;
#if SOLVER_USE_CACHE
    bool nodeSolved = true;
#endif

    const int start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    Board allMoves[6];
    int moves[6];
    int keys[6];
    int valid = 0;

    for (int i = start; i >= end; i--) {
        if (board->cells[i] == 0) continue;

        Board newBoard = *board;
        MAKE_MOVE(&newBoard, i);

        int k = FN(key)(&newBoard, board->color);
        if (i == previousBestMove) k += 100000;

        int j = valid;
        while (j > 0 && keys[j - 1] < k) {
            keys[j] = keys[j - 1];
            allMoves[j] = allMoves[j - 1];
            moves[j] = moves[j - 1];
            j--;
        }
        keys[j] = k;
        allMoves[j] = newBoard;
        moves[j] = i;
        valid++;
    }

    for (int i = 0; i < valid; i++) {
        int idx = i;
        if (i > 0 && valid > 2 && thread_id > 0) {
            idx = 1 + ((i - 1 + thread_id) % (valid - 1));
        }

        Board *boardCopy = &allMoves[idx];

#if SOLVER_USE_CACHE
        bool childSolved;
        if (board->color == boardCopy->color) {
            score = FN(negamax)(boardCopy, alpha, beta, depth - 1, &childSolved, thread_id);
        } else {
            score = -FN(negamax)(boardCopy, -beta, -alpha, depth - 1, &childSolved, thread_id);
        }
        nodeSolved = nodeSolved && childSolved;
#else
        if (board->color == boardCopy->color) {
            score = FN(negamax)(boardCopy, alpha, beta, depth - 1, thread_id);
        } else {
            score = -FN(negamax)(boardCopy, -beta, -alpha, depth - 1, thread_id);
        }
#endif

        if (score > reference) {
            reference = score;
            *bestMove = moves[idx];
        }

        alpha = max(alpha, reference);
        if (alpha >= beta) break;
    }
#if SOLVER_USE_CACHE
    *solved = nodeSolved;
#endif
    return reference;
}

#if SOLVER_USE_CACHE
void FN(distributionRoot)(Board *board, int *distribution, bool *solved, SolverConfig *config) {
#else
void FN(distributionRoot)(Board *board, int *distribution, bool *solvedOutput, SolverConfig *config) {
#endif
    atomic_store_explicit(&solver_abort_search, false, memory_order_relaxed);

    const int start = (board->color == 1) ? HBOUND_P1 : HBOUND_P2;
    const int end = (board->color == 1) ? LBOUND_P1 : LBOUND_P2;

    int index = 5;
    int score;

#if SOLVER_USE_CACHE
    int depth = config->depth;
    if (config->depth == 0) {
        depth = MAX_DEPTH;
        setCacheMode(false, config->compressCache);
    } else {
        setCacheMode(true, config->compressCache);
    }
#else
    int depth = (config->depth == 0) ? MAX_DEPTH : config->depth;
#endif

    const int alpha = config->clip ? 0 : INT32_MIN + 1;
    const int beta  = config->clip ? 1 : INT32_MAX;

#if SOLVER_USE_CACHE
    bool nodeSolved = true;
#else
    solved = true;
#endif

    for (int i = start; i >= end; i--) {
        if (board->cells[i] == 0) {
            distribution[index] = INT32_MIN;
            index--;
            continue;
        }

        Board boardCopy = *board;
        MAKE_MOVE(&boardCopy, i);

#if SOLVER_USE_CACHE
        bool childSolved;
        if (board->color == boardCopy.color) {
            score = FN(negamax)(&boardCopy, alpha, beta, depth, &childSolved, 0);
        } else {
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth, &childSolved, 0);
        }
        nodeSolved = nodeSolved && childSolved;
#else
        if (board->color == boardCopy.color) {
            score = FN(negamax)(&boardCopy, alpha, beta, depth, 0);
        } else {
            score = -FN(negamax)(&boardCopy, -beta, -alpha, depth, 0);
        }
#endif

        if (config->clip && score > 1) score = 1;

        distribution[index] = score;
        index--;
    }

#if SOLVER_USE_CACHE
    *solved = nodeSolved;
    stepCache();
    resetCacheStats();
#else
    *solvedOutput = solved;
#endif
}

// --- Thread Worker Logic ---
typedef struct {
    Context context;
    SolverConfig config;
    int thread_id;
} FN(ThreadWorkerArgs);

void* FN(aspirationWorker)(void* arg) {
    FN(ThreadWorkerArgs)* args = (FN(ThreadWorkerArgs)*)arg;
    nodeCount = 0;

    int currentDepth = 1;
    int alpha = INT32_MIN + 1;
    int beta  = INT32_MAX;

#if SOLVER_USE_CACHE
    bool oneShot = false;
    if (args->config.timeLimit == 0 && args->config.depth == 0) {
        currentDepth = MAX_DEPTH;
        oneShot = true;
    }
#endif

    while (!atomic_load_explicit(&solver_abort_search, memory_order_relaxed)) {
        bool solved = true;
        int bestMove = -1;
        bool searchValid = false;

        if (args->config.clip) {
#if SOLVER_USE_CACHE
            FN(negamaxWithMove)(args->context.board, &bestMove, 0, 1, currentDepth, &solved, -1, args->thread_id);
#else
            FN(negamaxWithMove)(args->context.board, &bestMove, 0, 1, currentDepth, -1, args->thread_id);
#endif
            searchValid = true;
        }
#if SOLVER_USE_CACHE
        else if (oneShot) {
            FN(negamaxWithMove)(args->context.board, &bestMove, INT32_MIN + 1, INT32_MAX, currentDepth, &solved, -1, args->thread_id);
            searchValid = true;
        }
#endif
        else {
#if SOLVER_USE_CACHE
            FN(negamaxWithMove)(args->context.board, &bestMove, alpha, beta, currentDepth, &solved, -1, args->thread_id);
#else
            FN(negamaxWithMove)(args->context.board, &bestMove, alpha, beta, currentDepth, -1, args->thread_id);
#endif
            searchValid = true;
        }

        if (searchValid) {
            if (solved) break;
#if SOLVER_USE_CACHE
            if (oneShot) break;
#endif
            if (args->config.depth > 0 && currentDepth >= args->config.depth) break;
            currentDepth += 1;
        }
    }
    
    atomic_fetch_add_explicit(&globalNodeCount, nodeCount, memory_order_relaxed);
    return NULL;
}

// --- Public: Aspiration Root ---
void FN(aspirationRoot)(Context* context, SolverConfig *config) {
    atomic_store_explicit(&solver_abort_search, false, memory_order_release);
    globalNodeCount = 0;
    nodeCount = 0;

    const int depthStep = 1;
    int currentDepth = 1;

    int bestMove = -1;
    int score = 0;
#if SOLVER_USE_CACHE
    bool oneShot = false;

    if (config->timeLimit == 0 && config->depth == 0) {
        currentDepth = MAX_DEPTH;
        oneShot = true;
        setCacheMode(false, config->compressCache);
    } else {
        setCacheMode(true, config->compressCache);
    }
    bool solved = false;
#endif

    const int windowSize = 1;
    int window = windowSize;

    int alpha = INT32_MIN + 1;
    int beta  = INT32_MAX;
    int windowMisses = 0;

    clock_t start = clock();

    double* depthTimes = context->metadata.lastDepthTimes;
    if (depthTimes != NULL) {
        for (int i = 0; i < MAX_DEPTH; i++) depthTimes[i] = -1.0;
    }
    double lastTimeCaptured = 0.0;

    startProgress(config, PLAY_PREFIX);

    int num_threads = config->threads > 0 ? config->threads : 1;
    pthread_t* threads = NULL;
    FN(ThreadWorkerArgs)* worker_args = NULL;

    if (num_threads > 1) {
        threads = malloc(sizeof(pthread_t) * (num_threads - 1));
        worker_args = malloc(sizeof(FN(ThreadWorkerArgs)) * (num_threads - 1));
        for (int i = 0; i < num_threads - 1; i++) {
            worker_args[i].context = *context;
            worker_args[i].config = *config;
            worker_args[i].thread_id = i + 1;
            pthread_create(&threads[i], NULL, FN(aspirationWorker), &worker_args[i]);
        }
    }

    while (true) {
#if SOLVER_USE_CACHE
        solved = true;
#else
        bool solved = true;
#endif
        bool searchValid = false;
        int previousBest = bestMove;

        if (config->clip) {
#if SOLVER_USE_CACHE
            score = FN(negamaxWithMove)(context->board, &bestMove, 0, 1, currentDepth, &solved, previousBest, 0);
#else
            score = FN(negamaxWithMove)(context->board, &bestMove, 0, 1, currentDepth, previousBest, 0);
#endif
            searchValid = true;
        }
#if SOLVER_USE_CACHE
        else if (oneShot) {
            score = FN(negamaxWithMove)(context->board, &bestMove, INT32_MIN + 1, INT32_MAX, currentDepth, &solved, previousBest, 0);
            searchValid = true;
        }
#endif
        else {
#if SOLVER_USE_CACHE
            score = FN(negamaxWithMove)(context->board, &bestMove, alpha, beta, currentDepth, &solved, previousBest, 0);
#else
            score = FN(negamaxWithMove)(context->board, &bestMove, alpha, beta, currentDepth, previousBest, 0);
#endif

            if (score > alpha && score < beta) {
                searchValid = true;
                window = windowSize;
                alpha = score - window;
                beta  = score + window;
            } else {
                windowMisses++;
                window *= 2;
                alpha = score - window;
                beta  = score + window;
            }
        }

#if SOLVER_USE_CACHE
        stepCache();
#endif

        if (searchValid) {
            int timeIndex = currentDepth;
#if SOLVER_USE_CACHE
            timeIndex = oneShot ? 1 : currentDepth;
#endif
            if (depthTimes != NULL) {
                double t = (double)(clock() - start) / CLOCKS_PER_SEC;
                depthTimes[timeIndex] = t - lastTimeCaptured;
                lastTimeCaptured = t;
            }

            atomic_fetch_add_explicit(&globalNodeCount, nodeCount, memory_order_relaxed);
            nodeCount = 0;

            updateProgress(currentDepth, bestMove, score, atomic_load_explicit(&globalNodeCount, memory_order_relaxed));

            if (solved) break;
#if SOLVER_USE_CACHE
            if (oneShot) break;
#endif
            if (config->depth > 0 && currentDepth >= config->depth) break;
            if (config->timeLimit > 0 && ((double)(clock() - start) / CLOCKS_PER_SEC) >= config->timeLimit) break;

            currentDepth += depthStep;
        }
    }

    atomic_store_explicit(&solver_abort_search, true, memory_order_release);
    
    if (num_threads > 1) {
        for (int i = 0; i < num_threads - 1; i++) {
            pthread_join(threads[i], NULL);
        }
        free(threads);
        free(worker_args);
    }

    finishProgress();
    atomic_fetch_add_explicit(&globalNodeCount, nodeCount, memory_order_relaxed);

    if (config->clip && score > 1) score = 1;

    context->metadata.lastTime = (double)(clock() - start) / CLOCKS_PER_SEC;
    context->metadata.lastNodes = atomic_load_explicit(&globalNodeCount, memory_order_relaxed);
    context->metadata.lastMove = bestMove;
    context->metadata.lastEvaluation = score;
    context->metadata.lastDepth = currentDepth;
#if SOLVER_USE_CACHE
    context->metadata.lastSolved = solved;
#else
    context->metadata.lastSolved = false;
#endif

    if (!config->clip && windowMisses > currentDepth) {
        renderOutput("[WARNING]: High window misses!", PLAY_PREFIX);
    }
    if (config->clip && score < 0) {
        renderOutput("[WARNING]: Clipped solver used in losing position!", CHEAT_PREFIX);
    }

#if SOLVER_USE_CACHE
    resetCacheStats();
#endif
}
