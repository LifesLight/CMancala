#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "containers.h"
#include "logic/board.h"
#include "user/render.h"
#include "logic/solver/troughput.h"
#include "logic/utility.h"
#include "logic/solver/common.h"
#include "logic/solver/solveCache.h"
#include "config.h"

#define DEBUG_CACHE

/**
 * Negamax algorithm with alpha-beta pruning.
*/
int LOCAL_negamax(
    Board *board, int alpha, int beta, const int depth, bool* solved);

/**
 * Negamax algorithm with alpha-beta pruning and best move output.
*/
int LOCAL_negamaxWithMove(
    Board *board, int *bestMove, int alpha, int beta, const int depth, bool* solved);

/**
 * Negamax with iterative deepening and aspiration window search.
*/
void LOCAL_negamaxAspirationRoot(Context* context);

/**
 * Negamax root with distribution.
 * Full search without any optimizations.
*/
void LOCAL_negamaxRootWithDistribution(Board *board, int depth, int32_t* distribution, bool* solved);
