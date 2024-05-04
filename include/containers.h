#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include <stdint.h>
#include <stdbool.h>

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

typedef struct {
    int stones;
    Distribution distribution;
    int seed;
    double timeLimit;
    int depth;
    bool startColor;
    bool autoplay;
    Agent player1;
    Agent player2;
} Config;

typedef struct {
    uint8_t cells[ASIZE];
    int8_t color;
} Board;

typedef struct {
    Board* board;
    Config* config;
    int lastEvaluation;
    int lastMove;
} Context;
