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
#include "logic/solver/cache.h"
#include "config.h"

/**
 * Negamax algorithm with alpha-beta pruning and best move output.
*/
int LOCAL_CLIP_negamaxWithMove(Board *board, int *bestMove, const int depth, bool *solved);

/**
 * Distribution root
*/
void LOCAL_CLIP_distributionRoot(Board *board, int *distribution, bool *solved, SolverConfig *config);

/**
 * Aspiration root
*/
void LOCAL_CLIP_aspirationRoot(Context* context, SolverConfig *config);
