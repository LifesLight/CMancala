#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "containers.h"
#include "logic/board.h"
#include "user/render.h"
#include "logic/throughput.h"
#include "logic/utility.h"
#include "logic/solver/cache.h"
#include "config.h"

// --- LOCAL Solver (With Cache) ---

void distributionRoot_LOCAL(Board *board, int *distribution, bool *solved, SolverConfig *config);
void aspirationRoot_LOCAL(Context* context, SolverConfig *config);

// --- GLOBAL Solver (No Cache) ---

void distributionRoot_GLOBAL(Board *board, int *distribution, bool *solved, SolverConfig *config);
void aspirationRoot_GLOBAL(Context* context, SolverConfig *config);
