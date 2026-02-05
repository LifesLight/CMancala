#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */

// TODO Solve caching per branch

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "containers.h"
#include "logic/board.h"
#include "user/render.h"
#include "config.h"
#include "logic/solver/full/global.h"
#include "logic/solver/clipped/global.h"
#include "logic/solver/full/local.h"
#include "logic/solver/clipped/local.h"

/**
 * Here are global algorithms, regardless the solver
*/

/**
 * Negamax algorithm with alpha-beta pruning and trace output.
 * trace array needs to be size of depth.
 * this ONLY works when depth and alpha beta are from a valid computation.
*/
NegamaxTrace traceRoot(
    Board *board, int alpha, const int beta, const int depth);

void aspirationRoot(Context* context, SolverConfig *solver);

void distributionRoot(Board *board, int32_t* distribution, bool *solved, SolverConfig *config);
