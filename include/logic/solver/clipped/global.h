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
#include "config.h"

/**
 * Negamax algorithm with alpha-beta pruning and best move output.
*/
int GLOBAL_CLIP_negamaxWithMove(
    Board *board, int *bestMove, int alpha, const int beta, const int depth);

/**
 * Distribution root
 */
void GLOBAL_CLIP_distributionRoot(Board *board, int *distribution, bool *solvedOutput, SolverConfig *config);

/**
 * Aspiration root
 */
void GLOBAL_CLIP_aspirationRoot(Context* context, SolverConfig *config);
