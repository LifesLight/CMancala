#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include <stdint.h>
#include <stdbool.h>

#include "config.h"

enum Distribution {
    UNIFORM,
    RANDOM
};

typedef struct {
    int stones;
    enum Distribution distribution;
    int seed;
    double timeLimit;
    int depth;
    bool startColor;
    bool autoplay;
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
