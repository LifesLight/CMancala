#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */


#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "containers.h"
#include "user/render.h"
#include "logic/utility.h"
#include "logic/solveCache.h"

void handleConfigInput(bool* requestedStart, Config* config);
