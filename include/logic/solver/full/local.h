#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "containers.h"
#include "logic/board.h"
#include "user/render.h"
#include "logic/throughput.h"
#include "logic/utility.h"
#include "logic/solver/solveCache.h"
#include "config.h"

/**
 * Negamax algorithm with alpha-beta pruning and best move output.
*/
int LOCAL_negamaxWithMove(
    Board *board, int *bestMove, int alpha, int beta, const int depth);

/**
 * Distribution root
*/
void LOCAL_distributionRoot(Board *board, int *distribution, SolverConfig *config);

/**
 * Aspiration root
*/
void LOCAL_aspirationRoot(Context* context, SolverConfig *config);
