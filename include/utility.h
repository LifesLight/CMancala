#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */


#include <stdio.h>

#include "config.h"
#include "containers.h"
#include "board.h"
#include "render.h"

void getInput(char* input, const char* prefix);
void initializeBoardFromConfig(Board* board, Config* config);
void quitGame();
void updateCell(Board* board, int player, int idx, int value);
