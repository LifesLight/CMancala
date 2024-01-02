#pragma once

/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "board.h"

bool solved;

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
int negamax(Board *board, int alpha, const int beta, const int depth);

/**
 * Negamax algorithm with alpha-beta pruning and best move output.
*/
int negamaxWithMove(
    Board *board, int *bestMove, int alpha, const int beta, const int depth);

/**
 * Negamax root with depth limit
*/
void negamaxRootDepth(Board *board, int *move, int *evaluation, int depth);

/**
 * Negamax root with time limit
*/
void negamaxRootTime(
    Board *board, int *move, int *evaluation, double timeInSeconds);
