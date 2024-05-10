#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "config.h"

typedef enum {
    UNIFORM_DIST,
    RANDOM_DIST
} Distribution;

typedef enum {
    HUMAN_AGENT,
    RANDOM_AGENT,
    AI_AGENT
} Agent;

typedef enum {
    CLASSIC_MOVE,
    AVALANCHE_MOVE
} MoveFunction;

typedef enum {
    QUICK_SOLVER,
    LOCAL_SOLVER,
    GLOBAL_SOLVER
} Solver;

typedef struct {
    int stones;
    Distribution distribution;
    int seed;
    double timeLimit;
    int depth;
    uint8_t startColor;
    bool autoplay;
    Agent player1;
    Agent player2;
    MoveFunction moveFunction;
    Solver solver;
    int quickSolverGoodEnough;
} Config;

typedef struct {
    uint8_t cells[ASIZE];
    int8_t color;
} Board;

typedef struct {
    int score;
    int8_t* moves;
} NegamaxTrace;

typedef struct {
    Board* board;
    Board* lastBoard;
    Config* config;
    int lastEvaluation;
    int lastMove;
    int lastDepth;
    bool lastSolved;
    double lastTime;
    uint64_t lastNodes;
} Context;
