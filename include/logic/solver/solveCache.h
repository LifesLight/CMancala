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
#include <math.h>

#include "config.h"
#include "logic/board.h"
#include "logic/utility.h"
#include "user/render.h"

#define UNSET_VALUE INT8_MIN
#define INVALID_HASH UINT64_MAX

#define SMALL_CACHE_SIZE    2500009
#define NORMAL_CACHE_SIZE   5000011
#define LARGE_CACHE_SIZE    10000019
#define EXTREME_CACHE_SIZE  25000009
#define OUTPUT_CHUNK_COUNT 25

#define EXACT_BOUND 0
#define LOWER_BOUND 1
#define UPPER_BOUND 2

/**
 * How many bits are used as validation for the cache.
 * 32 is very good, 64 is guaranteed no undetected.
 * If worried about specific performances compile with GUARANTEE_VALIDATION,
 * this will output how many undetected collisions occurred.
*/
#define VALIDATION_SIZE 64
// #define GUARANTEE_VALIDATION

#if VALIDATION_SIZE == 8
#define VALIDATION_TYPE uint8_t
#define VALMAX UINT8_MAX
#elif VALIDATION_SIZE == 16
#define VALIDATION_TYPE uint16_t
#define VALMAX UINT16_MAX
#elif VALIDATION_SIZE == 32
#define VALIDATION_TYPE uint32_t
#define VALMAX UINT32_MAX
#elif VALIDATION_SIZE == 64
#define VALIDATION_TYPE uint64_t
#define VALMAX UINT64_MAX 
#endif

/**
 * Specify how many compute iterations must pass before a cache entry is allowed to be overwritten.
 * If this is undefined this feature is disabled.
*/
// #define CACHE_GUARD 2

/**
 * Starts the cache.
*/
void startCache(uint32_t cacheSize);

/**
 * Caches a node.
*/
void cacheNode(Board* board, int evaluation, int boundType);

/**
 * Gets the value for the provided board from the cache.
 * Returns true if values where found
*/
bool getCachedValue(Board* board, int *evaluation, int *boundType);

/**
 * Gets the number of cached nodes.
*/
int getCachedNodeCount();

/**
 * Gets overviews of the cache stats.
*/
void renderCacheStats();

void stepCache();

uint32_t getCacheSize();
