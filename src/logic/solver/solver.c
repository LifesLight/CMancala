/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/solver.h"

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
