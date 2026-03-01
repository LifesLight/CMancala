/**
 * Copyright (c) Alexander Kurtz 2026
 */

#pragma once

#include "logic/board.h"
#include "logic/utility.h"

#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define EGDB_MAX_STONES 216
#define EGDB_UNCOMPUTED 127
#define EGDB_VISITING 126

extern int8_t* egdb_tables[EGDB_MAX_STONES + 1];
extern int loaded_egdb_max_stones;
extern int egdb_total_stones_configured;

void configureStoneCountEGDB(int stonesPerPit);
void generateEGDB(int max_stones);
void loadEGDB(int max_stones);
void freeEGDB();

bool EGDB_probe(Board* board, int* score);
void getEGDBStats(uint64_t* sizeBytes, uint64_t* hits, int* minStones, int* maxStones);
void resetEGDBStats();
