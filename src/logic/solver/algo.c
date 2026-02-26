/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/algo.h"
#include <stdatomic.h>

// --- Global Multithreading Variables ---
_Thread_local uint64_t nodeCount = 0;
_Atomic uint64_t globalNodeCount = 0;
_Atomic bool solver_abort_search = false;

// 1. INSTANTIATE LOCAL SOLVER CLASSIC (With Cache)
// FN(negamax) -> negamax_LOCAL_CLASSIC
#define PREFIX LOCAL_CLASSIC
#define SOLVER_USE_CACHE 1
#define MAKE_MOVE makeMoveOnBoardClassic
#include "logic/solver/impl/solver_template.h"
#undef PREFIX
#undef SOLVER_USE_CACHE
#undef MAKE_MOVE

// 2. INSTANTIATE LOCAL SOLVER AVALANCHE (With Cache)
// FN(negamax) -> negamax_LOCAL_AVALANCHE
#define PREFIX LOCAL_AVALANCHE
#define SOLVER_USE_CACHE 1
#define MAKE_MOVE makeMoveOnBoardAvalanche
#include "logic/solver/impl/solver_template.h"
#undef PREFIX
#undef SOLVER_USE_CACHE
#undef MAKE_MOVE

// 3. INSTANTIATE GLOBAL SOLVER CLASSIC (No Cache)
// FN(negamax) -> negamax_GLOBAL_CLASSIC
#define PREFIX GLOBAL_CLASSIC
#define SOLVER_USE_CACHE 0
#define MAKE_MOVE makeMoveOnBoardClassic
#include "logic/solver/impl/solver_template.h"
#undef PREFIX
#undef SOLVER_USE_CACHE
#undef MAKE_MOVE

// 4. INSTANTIATE GLOBAL SOLVER AVALANCHE (No Cache)
// FN(negamax) -> negamax_GLOBAL_AVALANCHE
#define PREFIX GLOBAL_AVALANCHE
#define SOLVER_USE_CACHE 0
#define MAKE_MOVE makeMoveOnBoardAvalanche
#include "logic/solver/impl/solver_template.h"
#undef PREFIX
#undef SOLVER_USE_CACHE
#undef MAKE_MOVE

extern void aspirationRoot_GLOBAL_CLASSIC(Context* context, SolverConfig *config);
extern void aspirationRoot_GLOBAL_AVALANCHE(Context* context, SolverConfig *config);
extern void aspirationRoot_LOCAL_CLASSIC(Context* context, SolverConfig *config);
extern void aspirationRoot_LOCAL_AVALANCHE(Context* context, SolverConfig *config);

extern void distributionRoot_GLOBAL_CLASSIC(Board *board, int32_t* distribution, bool *solved, SolverConfig *config);
extern void distributionRoot_GLOBAL_AVALANCHE(Board *board, int32_t* distribution, bool *solved, SolverConfig *config);
extern void distributionRoot_LOCAL_CLASSIC(Board *board, int32_t* distribution, bool *solved, SolverConfig *config);
extern void distributionRoot_LOCAL_AVALANCHE(Board *board, int32_t* distribution, bool *solved, SolverConfig *config);

void aspirationRoot(Context* context, SolverConfig *config) {
    config->threads = 4;
    switch (config->solver) {
        case GLOBAL_SOLVER:
            if (getMoveFunction() == CLASSIC_MOVE) {
                aspirationRoot_GLOBAL_CLASSIC(context, config);
            } else {
                aspirationRoot_GLOBAL_AVALANCHE(context, config);
            }
            break;
        case LOCAL_SOLVER:
            if (getMoveFunction() == CLASSIC_MOVE) {
                aspirationRoot_LOCAL_CLASSIC(context, config);
            } else {
                aspirationRoot_LOCAL_AVALANCHE(context, config);
            }
            break;
    }
}

void distributionRoot(Board *board, int32_t* distribution, bool *solved, SolverConfig *config) {
    switch (config->solver) {
        case GLOBAL_SOLVER:
            if (getMoveFunction() == CLASSIC_MOVE) {
                distributionRoot_GLOBAL_CLASSIC(board, distribution, solved, config);
            } else {
                distributionRoot_GLOBAL_AVALANCHE(board, distribution, solved, config);
            }
            break;
        case LOCAL_SOLVER:
            if (getMoveFunction() == CLASSIC_MOVE) {
                distributionRoot_LOCAL_CLASSIC(board, distribution, solved, config);
            } else {
                distributionRoot_LOCAL_AVALANCHE(board, distribution, solved, config);
            }
            break;
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

    if (depth == 0) {
        result.score = board->color * getBoardEvaluation(board);
        return result;
    }

    Board boardCopy;
    NegamaxTrace tempResult;

    const int start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    for (int i = start; i >= end; i--) {
        if (board->cells[i] == 0) continue;

        boardCopy = *board;
        makeMoveFunction(&boardCopy, i);

        if (board->color == boardCopy.color) {
            tempResult = negamaxWithTrace(&boardCopy, alpha, beta, depth - 1);
        } else {
            tempResult = negamaxWithTrace(&boardCopy, -beta, -alpha, depth - 1);
            tempResult.score = -tempResult.score;
        }

        if (tempResult.score > result.score) {
            result.score = tempResult.score;
            memcpy(result.moves, tempResult.moves, sizeof(int8_t) * depth);
            result.moves[depth - 1] = i;
        }

        free(tempResult.moves);
        alpha = max(alpha, result.score);

        if (alpha >= beta) break;
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

    if (depth == 0) {
        result.score = board->color * getBoardEvaluation(board);
        return result;
    }

    Board boardCopy;
    NegamaxTrace tempResult;

    const int start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    for (int i = start; i >= end; i--) {
        if (board->cells[i] == 0) continue;

        boardCopy = *board;
        makeMoveFunction(&boardCopy, i);

        if (board->color == boardCopy.color) {
            tempResult = negamaxWithTrace(&boardCopy, alpha, beta, depth - 1);
        } else {
            tempResult = negamaxWithTrace(&boardCopy, -beta, -alpha, depth - 1);
            tempResult.score = -tempResult.score;
        }

        if (tempResult.score > result.score) {
            result.score = tempResult.score;
            memcpy(result.moves, tempResult.moves, sizeof(int8_t) * depth);
            result.moves[depth - 1] = i;
        }

        free(tempResult.moves);
        alpha = max(alpha, result.score);

        if (alpha >= beta) break;
    }

    return result;
}
