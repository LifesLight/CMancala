#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */

/**
 * Caching algorithm:
 * We ignore score cells, this will allow for a more efficient caching since
 * they don't matter for the "goodness" of the playing board which is what we are caching.
 * This also allows for more reuse of the cache, since we can analyze the same board from different scores.
 * 
 * We do need to store the highest eval, it is NOT always the same with same input parameters since we are
 * ignoring score cells which can effect the alpha beta pruning.
 * 
 * Transposition table logic is from the following source:
 * https://en.wikipedia.org/wiki/Negamax
*/

#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>

#include "config.h"
#include "logic/board.h"
#include "logic/utility.h"
#include "user/render.h"

#define TINY_CACHE_SIZE     20
#define SMALL_CACHE_SIZE    22
#define NORMAL_CACHE_SIZE   24
#define LARGE_CACHE_SIZE    26
#define EXTREME_CACHE_SIZE  28

// <<-- PACKED BOUND + VAL -->>
#define CACHE_VAL_UNSET INT16_MAX

#define CACHE_VAL_MIN ((INT16_MIN >> 2) + 2)
#define CACHE_VAL_MAX (INT16_MAX >> 2)

#define EXACT_BOUND 0
#define LOWER_BOUND 1
#define UPPER_BOUND 2

#define DEPTH_SOLVED UINT16_MAX

// Does not allocate yet
void setCacheSize(int sizePow);


// Enable / Disable depth storing (needed for non solving) and can enable 48 bit board representations
void setCacheMode(bool depth, bool compress);


void invalidateCache();

void cacheNode(Board* board, int evaluation, int boundType, int depth, bool solved);

bool getCachedValue(Board* board, int currentDepth, int *evaluation, int *boundType, bool *solved);

void renderCacheStats();

void resetCacheStats();

void stepCache();

uint64_t getCacheSize();
