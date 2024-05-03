#pragma once

/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "containers.h"

// #define AVALANCHE


/**
 * Copies the board from the source to the target
*/
void copyBoard(const Board *board, Board *target);

/**
 * Configures the board with the given stones per cell
*/
void configBoard(Board *board, int stones);

/**
 * Configures the board with the given average stones per cell
 * and randomizes the board
*/
void configBoardRand(Board *board, const int stones, const int seed);

/**
 * Returns relative stone count of score cells
*/
int getBoardEvaluation(const Board *board);

/**
 * Check if the board is empty for player one
*/
bool isBoardPlayerOneEmpty(const Board *board);

/**
 * Check if the board is empty for player two
*/
bool isBoardPlayerTwoEmpty(const Board *board);

/**
 * Checks if empty for player one or two
 * If yes give remaining stones to opponent
*/
bool processBoardTerminal(Board *board);

/**
 * Makes a move on the board
*/
void makeMoveOnBoard(Board *board, const uint8_t actionIndex);

/**
 * Makes a move on the board
 * Also automatically handles terminal check
*/
void makeMoveManual(Board* board, int index);
