/**
 * Copyright (c) Alexander Kurtz 2026
 */

/**
 * This EGDB is strongly inspired by
 * https://github.com/girving/kalah
 */

#pragma once

#include "logic/board.h"
#include "logic/utility.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef LZ4
    #include <lz4.h>
    #include <lz4hc.h>
    #define EGDB_LZ4_MAGIC 0x48435A4C
    #define EGDB_LZ4_BLOCK_SIZE 1024
#endif
#define EGDB_MAX_STONES 216

#define EGDB_UNCOMPUTED 127
#define EGDB_VISITING 126

extern int8_t* egdb_tables[EGDB_MAX_STONES + 1];
extern int loaded_egdb_max_stones;
extern int egdb_total_stones_configured;

void configureStoneCountEGDB(int stonesPerPit);
void generateEGDB(int max_stones, bool compressed);
void loadEGDB(int max_stones);
void freeEGDB();

bool EGDB_probe(Board* board, int* score);
void getEGDBStats(uint64_t* sizeBytes, uint64_t* probes, uint64_t* hits, int* maxStones);
void resetEGDBStats();
