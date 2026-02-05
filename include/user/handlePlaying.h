#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "containers.h"
#include "config.h"
#include "logic/board.h"
#include "user/render.h"
#include "logic/solver/algo.h"
#include "logic/utility.h"

void stepGame(bool* requestedMenu, Context* context);
