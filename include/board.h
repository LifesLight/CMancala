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

typedef void (*MakeMoveFunction)(Board*, const uint8_t);

MakeMoveFunction makeMoveFunction;

/**
 * Sets the move function to use
*/
void setMoveFunction(MoveFunction moveFunction);

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
void configBoardRand(Board *board, const int stones);

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
void makeMoveOnBoardClassic(Board *board, const uint8_t actionIndex);
void makeMoveOnBoardAvalanche(Board *board, const uint8_t actionIndex);

/**
 * Makes a move on the board
 * Also automatically handles terminal check
*/
void makeMoveManual(Board* board, int index);

/**
 * Returns a reversible hash of the board if total number of stones is known.
 * Also only guaranteed to be unique per board if uniform distribution with <= 5 stones per cell.
 * Also dont't know about avalanche mode
*/
uint64_t hashBoard(const Board *board);

/**
 * Loads a board from a packed hash
 * Requires the total number of stones to be known
*/
void loadBoard(Board *board, const uint64_t hash, const uint8_t total);
