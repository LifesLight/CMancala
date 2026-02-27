#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */


#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "config.h"
#include "containers.h"
#include "logic/board.h"
#include "user/render.h"

#ifndef _WIN32
int min(const int a, const int b);
int max(const int a, const int b);
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0777)
#endif

void getInput(char* input, const char* prefix);
void initializeBoardFromConfig(Board* board, Config* config);
void quitGame();
void updateCell(Board* board, int player, int idx, int value);
void getLogNotation(char* buffer, uint64_t value);
void storeBenchmarkData(const char* fileName, double* data);
