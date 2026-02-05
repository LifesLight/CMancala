#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "containers.h"

/*
 * Board is constructed like this:
 * Score cells are 6 and 13
 * Indicies P1: 0 - 5
 * Indicies P2: 7 - 12
 */

typedef void (*MakeMoveFunction)(Board*, const uint8_t);

extern MakeMoveFunction makeMoveFunction;

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
 * Checks if game is over without handling terminal
*/
bool isBoardTerminal(const Board *board);

/**
 * Makes a move on the board
 * Also automatically handles terminal check
*/
void makeMoveManual(Board* board, int index);

/**
 * Returns a reversible encoding of the board.
*/
char* encodeBoard(const Board *board);

/**
 * Loads a board from a reversible encoding.
 * Returns success
*/
bool decodeBoard(Board *board, const char* code);
