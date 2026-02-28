#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

#include "containers.h"
#include "logic/utility.h"
#include "logic/solver/egdb/core.h"

#ifdef _WIN32
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
#define PLAYER_INDICATOR "|-> "
#define BAR_FILL "#"
#define BAR_EMPTY "-"
#define BAR_CAP_L "["
#define BAR_CAP_R "]"
#define STAT_SEP "|"
#else
#define VL "│"
#define HL "─"
#define TL "┌"
#define TR "┐"
#define BL "└"
#define BR "┘"
#define EL "├"
#define ER "┤"
#define ET "┬"
#define EB "┴"
#define CR "┼"
#define PLAYER_INDICATOR "┠─▶ "
#define BAR_FILL "█"
#define BAR_EMPTY "░"
#define BAR_CAP_L "▐"
#define BAR_CAP_R "▌"
#define STAT_SEP "│"
#endif

void renderBoard(const Board *board, const char* prefix, const GameSettings* settings);
void renderCustomBoard(const int32_t *cells, const int8_t color, const char* prefix, const GameSettings* settings);
void renderWelcome();
void renderOutput(const char* message, const char* prefix);
void renderCacheOverview(const CacheStats* stats, bool showFrag, bool showStoneDist, bool showDepthDist);
void renderEGDBOverview();

void startProgress(const SolverConfig* config, const char* prefix);
void updateProgress(int currentDepth, int bestMove, int score, uint64_t nodeCount);
void finishProgress();

void startEGDBProgress();
void updateEGDBProgress(int stones, uint64_t current, uint64_t total);
void finishEGDBProgress();
