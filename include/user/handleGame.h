#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "containers.h"
#include "config.h"
#include "logic/board.h"
#include "user/render.h"
#include "logic/algo.h"
#include "logic/utility.h"
#include "user/handlePlaying.h"

void startGameHandling(Config* config);
