#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include <stdint.h>

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
    int startColor;
    bool autoplay;
} Config;

typedef struct {
    uint8_t cells[ASIZE];
    int8_t color;
} Board;
