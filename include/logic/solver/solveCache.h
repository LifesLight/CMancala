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

/**
 * We are caching into a 64bit integer
 * 
 * The board gets encoded like:
 * Score cells don't get encoded
 * 
 * Current player gets 1 bit                : 1
 * 1 bit is reserved for valid hash         : 2
 * Playing cells get: 5, 5, 5, 5, 5, 5 bits : 62
 * Ignored: 2 bits                          : 64
*/

#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include "config.h"
#include "logic/board.h"
#include "logic/utility.h"
#include "user/render.h"

#define BUCKET_POW 1
// 2^BUCKET_POW
#define BUCKET_ELEMENTS 2

#define TINY_CACHE_SIZE     20
#define SMALL_CACHE_SIZE    22
#define NORMAL_CACHE_SIZE   24
#define LARGE_CACHE_SIZE    26
#define EXTREME_CACHE_SIZE  28

#define OUTPUT_CHUNK_COUNT 10

#define EXACT_BOUND 0
#define LOWER_BOUND 1
#define UPPER_BOUND 2

#define UNSET_VALIDATION UINT64_MAX
#define INVALID_HASH UINT64_MAX

/**
 * Starts the cache.
 * We specify the power of two that is the cache size.
*/
void startCache(int sizePow);

/**
 * Caches a node.
*/
void cacheNode(Board* board, int evaluation, int boundType, int depth, bool solved, int bestMove);

/**
 * Gets the value for the provided board from the cache.
 * Returns true if values where found
 * Current depth is the depth of the node querrying the cache.
*/
bool getCachedValue(Board* board, int currentDepth, int *evaluation, int *boundType, bool *solved, int *bestMove);

/**
 * Gets the number of cached nodes.
*/
int getCachedNodeCount();

/**
 * Gets overviews of the cache stats.
*/
void renderCacheStats();

void stepCache();

void resetCacheStats();

uint32_t getCacheSize();
