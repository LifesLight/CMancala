#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */


/**
 * Caching algorithm:
 * We ignore score cells, this will allow for a more efficient caching since.
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

#include "config.h"
#include "logic/board.h"
#include "logic/utility.h"
#include "user/render.h"

#define TINY_CACHE_SIZE     20
#define SMALL_CACHE_SIZE    22
#define NORMAL_CACHE_SIZE   24
#define LARGE_CACHE_SIZE    26
#define EXTREME_CACHE_SIZE  28

#define OUTPUT_CHUNK_COUNT 10

#define EXACT_BOUND 0
#define LOWER_BOUND 1
#define UPPER_BOUND 2

// Max stones per cell for encoding
#define CACHE_SPC_DEPTH    31
#define CACHE_SPC_NO_DEPTH 15

#define CACHE_VAL_UNSET INT16_MIN
#define CACHE_VAL_MIN (INT16_MIN + 1)
#define CACHE_VAL_MAX INT16_MAX

// Packed values for NO_DEPTH mode (2 bits stolen for bound type)
#define CACHE_VAL_MAX_PACKED (INT16_MAX >> 2)
#define CACHE_VAL_MIN_PACKED ((INT16_MIN >> 2) + 2)

#define DEPTH_SOLVED UINT16_MAX

/**
 * Starts the cache.
 * We specify the power of two that is the cache size.
*/
void startCache(int sizePow);

/**
 * Cleans cache, needed when changing move function for example.
 */
void invalidateCache();

/**
 * Toggles the cache mode.
 * true  = NO_DEPTH mode (smaller memory footprint, ignores depth).
 * false = DEPTH mode (normal behavior).
 * This resets the cache.
 */
void setCacheNoDepth(bool enable);

/**
 * Caches a node.
*/
void cacheNode(Board* board, int evaluation, int boundType, int depth, bool solved);

/**
 * Gets the value for the provided board from the cache.
 * Returns true if values where found
 * Current depth is the depth of the node querrying the cache.
*/
bool getCachedValue(Board* board, int currentDepth, int *evaluation, int *boundType, bool *solved);

/**
 * We are caching into a 64bit integer
 * 
 * The board gets encoded like:
 * Score cells don't get encoded
 * 
 * 1 bit reserved for valid is valid hash   : 1
 * Playing cells get: 5, 5, 5, 5, 5, 5 bits : 61
 * Ignored: 3 bits                          : 64
*/
bool translateBoard(Board* board, uint64_t *code);

/**
 * Gets overviews of the cache stats.
*/
void renderCacheStats();

void stepCache();

void resetCacheStats();

uint64_t getCacheSize();
