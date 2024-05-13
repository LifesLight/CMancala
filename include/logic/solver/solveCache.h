#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
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
 * Compute sum of playing cells 
*/

/**
 * We are caching into a 64bit integer
 * 
 * The board gets encoded like:
 * Score cells don't get encoded
 * 
 * Current player gets 1 bit
 * Playing cells get 5 bits each
 * 1 bit is reserved for valid hash
 * Remaining 2 bits are not used
*/

#include <stdint.h>
#include <math.h>

#include "config.h"
#include "logic/board.h"
#include "logic/utility.h"
#include "user/render.h"

#define UNSET_VALUE INT8_MIN
#define INVALID_HASH UINT64_MAX
#define NOT_CACHED_VALUE INT32_MIN

#define SMALL_CACHE_SIZE    100003
#define NORMAL_CACHE_SIZE   1000003
#define LARGE_CACHE_SIZE    10000019
#define OUTPUT_CHUNK_COUNT 25

/**
 * Starts the cache.
*/
void startCache(uint32_t cacheSize);

/**
 * Caches a node.
*/
void cacheNode(Board* board, int evaluation);

/**
 * Gets a cached value.
*/
int getCachedValue(Board* board);

/**
 * Gets the number of cached nodes.
*/
int getCachedNodeCount();

/**
 * Gets overviews of the cache stats.
*/
void renderCacheStats();

uint32_t getCacheSize();
