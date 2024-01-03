#pragma once

/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "board.h"

#define N_INFINITY -10000
#define P_INFINITY  10000


/**
 * Returns the maximum of two integers.
*/
float max(const float a, const float b);

/**
 * Returns the minimum of two integers.
*/
float min(const float a, const float b);

/**
 * Negamax algorithm with alpha-beta pruning.
*/
float negamax(Board *board, float alpha, const float beta, const int depth);

/**
 * Negamax algorithm with alpha-beta pruning and best move output.
*/
float negamaxWithMove(
    Board *board, int *bestMove, float alpha, const float beta, const int depth);

/**
 * Negamax root with depth limit
*/
void negamaxRootDepth(Board *board, int *move, float *evaluation, int depth);

/**
 * Negamax root with time limit
*/
void negamaxRootTime(
    Board *board, int *move, float *evaluation, double timeInSeconds);
