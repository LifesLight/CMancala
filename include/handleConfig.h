#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */


#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "containers.h"
#include "render.h"
#include "utility.h"

void handleConfigInput(bool* requestedQuit, bool* requestedStart, Config* config);
