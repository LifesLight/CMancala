#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */


#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "config.h"
#include "containers.h"
#include "logic/board.h"
#include "user/render.h"

void getInput(char* input, const char* prefix);
void initializeBoardFromConfig(Board* board, Config* config);
void quitGame();
void updateCell(Board* board, int player, int idx, int value);
