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
    LOCAL_SOLVER,
    GLOBAL_SOLVER
} Solver;

typedef struct {
    Solver solver;
    int depth;
    double timeLimit;
    uint32_t goodEnough;
} SolverConfig;

typedef struct {
    int stones;
    Distribution distribution;
    int seed;
    uint8_t startColor;
    Agent player1;
    Agent player2;
    MoveFunction moveFunction;
} GameSettings;

typedef struct {
    bool autoplay;
    GameSettings gameSettings;
    SolverConfig solverConfig;
} Config;

typedef struct {
    int lastEvaluation;
    int lastMove;
    int lastDepth;
    bool lastSolved;
    double lastTime;
    uint64_t lastNodes;
} Metadata;

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
    Config config;
    Metadata metadata;
} Context;
