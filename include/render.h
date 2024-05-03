#pragma once

/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"

#ifdef _WIN32
// Windows cant always render unicode
#define HL "-"
#define VL "|"
#define TL "+"
#define TR "+"
#define BL "+"
#define BR "+"
#define EL "+"
#define ER "+"
#define ET "+"
#define EB "+"
#define CR "+"
#else
// Unicode characters for rendering
#define VL "│"  // Vertical Line
#define HL "─"  // Horizontal Line
#define TL "┌"  // Top Left Corner
#define TR "┐"  // Top Right Corner
#define BL "└"  // Bottom Left Corner
#define BR "┘"  // Bottom Right Corner
#define EL "├"  // Edge Left
#define ER "┤"  // Edge Right
#define ET "┬"  // Edge Top
#define EB "┴"  // Edge Bottom
#define CR "┼"  // Cross
#endif

#define OUTPUT_PREFIX    ">> "
#define INPUT_PREFIX     "<< "
#define CONFIG_PREFIX    "(C) "
#define GAME_PREFIX      "(G) "

/**
 * Renders the board to the console.
*/
void renderBoard(const Board *board);
void renderWelcome();
void renderConfigHelp();
void renderOutput(const char* message, const char* prefix);