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

/**
 * Distribution root
 */
void LOCAL_distributionRoot(Board *board, int *distribution, bool *solved, SolverConfig *config);

/**
 * Aspiration root
 */
void LOCAL_aspirationRoot(Context* context, SolverConfig *config);
