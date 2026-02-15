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

#define OUTPUT_CHUNK_COUNT 10

// <<-- PACKED BOUND + VAL -->>
// Used to mark an entry as empty
#define CACHE_VAL_UNSET INT16_MAX

// Constraints for values to fit into packed 14-bit integer
#define CACHE_VAL_MIN ((INT16_MIN >> 2) + 2)
#define CACHE_VAL_MAX (INT16_MAX >> 2)

#define EXACT_BOUND 0
#define LOWER_BOUND 1
#define UPPER_BOUND 2

#define DEPTH_SOLVED UINT16_MAX

/**
 * Sets the size of the cache (power of two).
 * If sizePow <= 2, the cache is disabled and memory freed.
 * If the size changes, the cache is cleared and reallocated.
 */
void setCacheSize(int sizePow);

/**
 * Configures the cache operation mode.
 * 
 * @param depth    If true, stores search depth (16 bytes/entry). 
 *                 If false, ignores depth for tighter packing (8 bytes/entry).
 * @param compress If true, uses 4 bits per hole (Max stones 15, Key 48-bit).
 *                 If false, uses 5 bits per hole (Max stones 31, Key 64-bit).
 * 
 * Note: This may trigger a cache reset/reallocation if the mode changes.
 */
void setCacheMode(bool depth, bool compress);

/**
 * Cleans cache. Needed when changing game rules or cache logic explicitly.
 * Effectively re-initializes the current buffer.
 */
void invalidateCache();

/**
 * Caches a node.
 * Automatically handles replacement strategies (Depth-based or LRU) 
 * depending on the active mode.
 */
void cacheNode(Board* board, int evaluation, int boundType, int depth, bool solved);

/**
 * Gets the value for the provided board from the cache.
 * Returns true if values were found.
 * Current depth is the depth of the node querying the cache.
 */
bool getCachedValue(Board* board, int currentDepth, int *evaluation, int *boundType, bool *solved);

/**
 * Translates a board into a hash key.
 * 
 * Encoding depends on compression mode:
 * - Compress (B48): 12 holes * 4 bits = 48 bits. (Max 15 stones/hole)
 * - Standard (B64): 12 holes * 5 bits = 60 bits. (Max 31 stones/hole)
 * 
 * Score cells are not encoded.
 * Returns false if the board cannot be encoded (e.g. too many stones for the selected mode).
 */
bool translateBoard(Board* board, uint64_t *code);

/**
 * Outputs formatted cache statistics to the render output.
 */
void renderCacheStats();

/**
 * Resets the hit/miss/collision counters without clearing the actual cache memory.
 */
void resetCacheStats();

/**
 * Helper step function (reserved for aging or per-turn maintenance).
 */
void stepCache();

/**
 * Returns the current size of the cache in number of entries.
 */
uint64_t getCacheSize();
