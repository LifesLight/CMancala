#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "board.h"
#include "render.h"
#include "algo.h"

enum Distribution {
    UNIFORM,
    RANDOM
};

struct Config {
    int stones;
    enum Distribution distribution;
    int seed;
    double timeLimit;
    int depth;
    int startColor;
};

void startInterface();
