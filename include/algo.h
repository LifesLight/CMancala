#pragma once

/**
 * Copyright (c) Alexander Kurtz 2023
 */

// TODO Solve caching per branch

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "containers.h"
#include "board.h"
#include "render.h"
#include "config.h"

#ifdef TRACK_THROUGHPUT
extern int64_t nodes;
#endif

/**
 * Returns the maximum of two integers.
*/
int max(const int a, const int b);

/**
 * Returns the minimum of two integers.
*/
int min(const int a, const int b);

/**
 * Negamax algorithm with alpha-beta pruning.
*/
int negamax(
    Board *board, int alpha, const int beta, const int depth, bool* solved);

/**
 * Negamax algorithm with alpha-beta pruning and best move output.
*/
int negamaxWithMove(
    Board *board, int *bestMove, int alpha, const int beta, const int depth, bool* solved);

/**
 * Negamax algorithm with alpha-beta pruning and trace output.
 * trace array needs to be size of depth.
 * this ONLY works when depth and alpha beta are from a valid computation.
*/
NegamaxTrace negamaxWithTrace(
    Board *board, int alpha, const int beta, const int depth);

/**
 * Negamax with iterative deepening and aspiration window search.
*/
void negamaxAspirationRoot(Context* context);

/**
 * Negamax root with distribution.
 * Full search without any optimizations.
*/
void negamaxRootWithDistribution(
    Board *board, int depth, int32_t* distribution, bool* solved);
