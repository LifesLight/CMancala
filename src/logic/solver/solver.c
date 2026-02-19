/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/solver.h"

// 1. INSTANTIATE LOCAL SOLVER (With Cache)
// FN(negamax) -> negamax_LOCAL
#define PREFIX LOCAL
#define SOLVER_USE_CACHE 1
#include "logic/solver/impl/solver_template.h"
#undef PREFIX
#undef SOLVER_USE_CACHE

// 2. INSTANTIATE GLOBAL SOLVER (No Cache)
// FN(negamax) -> negamax_GLOBAL
#define PREFIX GLOBAL
#define SOLVER_USE_CACHE 0
#include "logic/solver/impl/solver_template.h"
#undef PREFIX
#undef SOLVER_USE_CACHE
