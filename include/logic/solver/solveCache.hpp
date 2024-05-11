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
 * 
 * Compute sum of playing cells 
*/

/**
 * We are caching into a 64bit integer
 * 
 * The board gets encoded like:
 * Score cells don't get encoded
 * Playing cells get 5 bits
 * Current player gets 1 bit
*/

#include <stdint.h>

#include "config.h"
#include "logic/board.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hashes a compatible board into a 64bit integer.
*/
uint64_t hashBoard(Board* board);

/**
 * Starts the cache.
*/
void startCache();

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

#ifdef __cplusplus
}
#endif
