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
#include "config.h"

/**
 * Negamax algorithm with alpha-beta pruning.
*/
int GLOBAL_negamax(
    Board *board, int alpha, const int beta, const int depth);

/**
 * Negamax algorithm with alpha-beta pruning and best move output.
*/
int GLOBAL_negamaxWithMove(
    Board *board, int *bestMove, int alpha, const int beta, const int depth);

/**
 * Negamax with iterative deepening and aspiration window search.
*/
void GLOBAL_negamaxAspirationRoot(Context* context);

/**
 * Negamax root with distribution.
 * Full search without any optimizations.
*/
void GLOBAL_negamaxRootWithDistribution(Board *board, int depth, int32_t* distribution, bool *solved);
