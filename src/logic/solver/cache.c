/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

// --- Global State & Stats ---
uint64_t cacheSize = 0;
uint32_t cacheSizePow = 0;

// Stats counters
uint64_t hits = 0;
uint64_t hitsLegalDepth = 0;
uint64_t sameKeyOverwriteCount = 0;
uint64_t victimOverwriteCount = 0;
uint64_t swapLRUCount = 0;
uint64_t failedEncodeStoneCount = 0;
uint64_t failedEncodeValueRange = 0;

// Snapshot stats
uint64_t lastHits = 0;
uint64_t lastHitsLegalDepth = 0;
uint64_t lastSameKeyOverwriteCount = 0;
uint64_t lastVictimOverwriteCount = 0;
uint64_t lastSwapLRUCount = 0;
uint64_t lastFailedEncodeStoneCount = 0;
uint64_t lastFailedEncodeValueRange = 0;

// --- Helper ---
typedef struct {
    uint64_t start;
    uint64_t size;
    int type;
} Chunk;

int compareChunksSize(const void *a, const void *b) {
    Chunk* cA = (Chunk*)a;
    Chunk* cB = (Chunk*)b;
    if (cA->size < cB->size) return 1;
    if (cA->size > cB->size) return -1;
    return 0;
}

int compareChunksStart(const void *a, const void *b) {
    Chunk* cA = (Chunk*)a;
    Chunk* cB = (Chunk*)b;
    if (cA->start > cB->start) return 1;
    if (cA->start < cB->start) return -1;
    return 0;
}

// --- Common Logic Macros ---
#define PACK_VALUE(eval, bt) ((int16_t)(((eval) << 2) | ((bt) & 0x3)))
#define UNPACK_VALUE(val) ((int16_t)((val) >> 2))
#define UNPACK_BOUND(val) ((val) & 0x3)

// --- Template Instantiations ---

// 1. NO DEPTH | 48 BIT KEY | 16 BIT TAG
#define PREFIX NOCACHE_B48_T16
#define CACHE_DEPTH 0
#define CACHE_B64 0
#define CACHE_T32 0
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B64
#undef CACHE_T32

// 2. NO DEPTH | 48 BIT KEY | 32 BIT TAG
#define PREFIX NOCACHE_B48_T32
#define CACHE_DEPTH 0
#define CACHE_B64 0
#define CACHE_T32 1
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B64
#undef CACHE_T32

// 3. NO DEPTH | 64 BIT KEY | 32 BIT TAG (B64 requires T32)
#define PREFIX NOCACHE_B64_T32
#define CACHE_DEPTH 0
#define CACHE_B64 1
#define CACHE_T32 1
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B64
#undef CACHE_T32

// 4. DEPTH | 48 BIT KEY | 16 BIT TAG
#define PREFIX CACHE_DEPTH_B48_T16
#define CACHE_DEPTH 1
#define CACHE_B64 0
#define CACHE_T32 0
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B64
#undef CACHE_T32

// 5. DEPTH | 48 BIT KEY | 32 BIT TAG
#define PREFIX CACHE_DEPTH_B48_T32
#define CACHE_DEPTH 1
#define CACHE_B64 0
#define CACHE_T32 1
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B64
#undef CACHE_T32

// 6. DEPTH | 64 BIT KEY | 32 BIT TAG
#define PREFIX CACHE_DEPTH_B64_T32
#define CACHE_DEPTH 1
#define CACHE_B64 1
#define CACHE_T32 1
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B64
#undef CACHE_T32

// --- Dispatcher ---

typedef enum {
    MODE_DISABLED = -1,
    MODE_ND_B48_T16 = 0,
    MODE_ND_B48_T32,
    MODE_ND_B64_T32,
    MODE_D_B48_T16,
    MODE_D_B48_T32,
    MODE_D_B64_T32,
    MODE_COUNT
} CacheMode;

// Current Configuration
static CacheMode currentMode = MODE_DISABLED;
static bool configDepth = true;
static bool configCompress = true;
static int configSizePow = 0;

// --- Internal Logic ---

void resetCacheStats() {
    lastHits = hits;
    lastHitsLegalDepth = hitsLegalDepth;
    lastSameKeyOverwriteCount = sameKeyOverwriteCount;
    lastVictimOverwriteCount = victimOverwriteCount;
    lastFailedEncodeStoneCount = failedEncodeStoneCount;
    lastFailedEncodeValueRange = failedEncodeValueRange;
    lastSwapLRUCount = swapLRUCount;

    hits = 0;
    hitsLegalDepth = 0;
    sameKeyOverwriteCount = 0;
    victimOverwriteCount = 0;
    failedEncodeStoneCount = 0;
    failedEncodeValueRange = 0;
    swapLRUCount = 0;
}

static void freeCurrentCache() {
    switch (currentMode) {
        case MODE_ND_B48_T16: freeCacheInternal_NOCACHE_B48_T16(); break;
        case MODE_ND_B48_T32: freeCacheInternal_NOCACHE_B48_T32(); break;
        case MODE_ND_B64_T32: freeCacheInternal_NOCACHE_B64_T32(); break;
        case MODE_D_B48_T16:  freeCacheInternal_CACHE_DEPTH_B48_T16(); break;
        case MODE_D_B48_T32:  freeCacheInternal_CACHE_DEPTH_B48_T32(); break;
        case MODE_D_B64_T32:  freeCacheInternal_CACHE_DEPTH_B64_T32(); break;
        default: break;
    }
    currentMode = MODE_DISABLED;
}

/**
 * Calculates the required mode based on configuration and allocates memory.
 * Returns true on success, or quits the game on impossible config.
 */
static void reconfigureCache() {
    if (currentMode != MODE_DISABLED) {
        freeCurrentCache();
    }

    if (configSizePow <= 2) {
        // Disabled
        cacheSize = 0;
        cacheSizePow = 0;
        return;
    }
    int keyBits = configCompress ? 48 : 64;

    // Check if too large cache (highly unlikely on modern systems, but safety check)
    // We reserve at least 1 bit for index, so sizePow < keyBits
    if (configSizePow >= keyBits) {
        char err[128];
        snprintf(err, sizeof(err), "Fatal: Cache size 2^%d too large for %d-bit keys.", configSizePow, keyBits);
        renderOutput(err, CONFIG_PREFIX);
        quitGame();
        return;
    }

    // Bucket Logic: We have 2 entries per bucket.
    // Addressable Buckets = (1 << configSizePow) / 2 = 1 << (configSizePow - 1).
    // Index Bits = configSizePow - 1.
    int indexBits = configSizePow - 1;
    int tagBitsNeeded = keyBits - indexBits;
    
    bool useT32 = false;

    // Check bounds
    if (tagBitsNeeded > 32) {
        // Because the cache size determines the index bits, a small cache requires a large tag.
        // For 64-bit keys (B64), we need a very large cache to fit the rest into 32 bits.
        // indexBits must be >= 32, so SizePow >= 33.
        char err[128];
        snprintf(err, sizeof(err), "Fatal: Cache size 2^%d too small for %d-bit keys. Tag would require %d bits (32 max, need 2^%d min cache).", 
                 configSizePow, keyBits, tagBitsNeeded, keyBits - 32 + 1);
        renderOutput(err, CONFIG_PREFIX);
        quitGame();
        return;
    }
    else if (tagBitsNeeded > 16) {
        useT32 = true;
    }
    else {
        // Fallback or preference
        // If we don't compile B64 T16, we force T32 for B64 even if it fit (it won't fit usually).
        // But for B48, if it fits in 16, we can use T16.
        if (configCompress) {
            useT32 = false;
        } else {
            // Non-compressed (B64) always uses T32 because we don't instantiate B64_T16
            useT32 = true;
        }
    }

    // Select Mode
    if (configDepth) {
        if (!configCompress) {
             currentMode = MODE_D_B64_T32;
        } else {
             currentMode = useT32 ? MODE_D_B48_T32 : MODE_D_B48_T16;
        }
    } else {
        if (!configCompress) {
             currentMode = MODE_ND_B64_T32;
        } else {
             currentMode = useT32 ? MODE_ND_B48_T32 : MODE_ND_B48_T16;
        }
    }

    // 4. Allocate
    cacheSize = (uint64_t)1 << configSizePow;
    cacheSizePow = configSizePow;

    // 5. Initialize selected mode
    switch (currentMode) {
        case MODE_ND_B48_T16: initCacheInternal_NOCACHE_B48_T16(cacheSize); break;
        case MODE_ND_B48_T32: initCacheInternal_NOCACHE_B48_T32(cacheSize); break;
        case MODE_ND_B64_T32: initCacheInternal_NOCACHE_B64_T32(cacheSize); break;
        case MODE_D_B48_T16:  initCacheInternal_CACHE_DEPTH_B48_T16(cacheSize); break;
        case MODE_D_B48_T32:  initCacheInternal_CACHE_DEPTH_B48_T32(cacheSize); break;
        case MODE_D_B64_T32:  initCacheInternal_CACHE_DEPTH_B64_T32(cacheSize); break;
        default: break;
    }

    resetCacheStats();
}

// --- Public API ---

void setCacheMode(bool depth, bool compress) {
    bool sizeChanged = (configSizePow != (int)cacheSizePow);
    bool modeChanged = (configDepth != depth) || (configCompress != compress);

    configDepth = depth;
    configCompress = compress;

    if (!sizeChanged && !modeChanged) {
        return;
    }

    if (configSizePow > 0) {
        reconfigureCache();
    }
}

void setCacheSize(int sizePow) {
    configSizePow = sizePow;
}

void cacheNode(Board* board, int evaluation, int boundType, int depth, bool solved) {
    switch (currentMode) {
        case MODE_ND_B48_T16: cacheNode_NOCACHE_B48_T16(board, evaluation, boundType, depth, solved); break;
        case MODE_ND_B48_T32: cacheNode_NOCACHE_B48_T32(board, evaluation, boundType, depth, solved); break;
        case MODE_ND_B64_T32: cacheNode_NOCACHE_B64_T32(board, evaluation, boundType, depth, solved); break;
        case MODE_D_B48_T16:  cacheNode_CACHE_DEPTH_B48_T16(board, evaluation, boundType, depth, solved); break;
        case MODE_D_B48_T32:  cacheNode_CACHE_DEPTH_B48_T32(board, evaluation, boundType, depth, solved); break;
        case MODE_D_B64_T32:  cacheNode_CACHE_DEPTH_B64_T32(board, evaluation, boundType, depth, solved); break;
        default: break;
    }
}

bool getCachedValue(Board* board, int currentDepth, int *evaluation, int *boundType, bool *solved) {
    switch (currentMode) {
        case MODE_ND_B48_T16: return getCachedValue_NOCACHE_B48_T16(board, currentDepth, evaluation, boundType, solved);
        case MODE_ND_B48_T32: return getCachedValue_NOCACHE_B48_T32(board, currentDepth, evaluation, boundType, solved);
        case MODE_ND_B64_T32: return getCachedValue_NOCACHE_B64_T32(board, currentDepth, evaluation, boundType, solved);
        case MODE_D_B48_T16:  return getCachedValue_CACHE_DEPTH_B48_T16(board, currentDepth, evaluation, boundType, solved);
        case MODE_D_B48_T32:  return getCachedValue_CACHE_DEPTH_B48_T32(board, currentDepth, evaluation, boundType, solved);
        case MODE_D_B64_T32:  return getCachedValue_CACHE_DEPTH_B64_T32(board, currentDepth, evaluation, boundType, solved);
        default: return false;
    }
}

uint64_t getCacheSize() {
    if (configSizePow <= 2) return 0;
    return (uint64_t)1 << configSizePow;
}

void stepCache() {
    // Not used in this version
}

void invalidateCache() {
    if (currentMode != MODE_DISABLED && cacheSize > 0) {
        switch (currentMode) {
            case MODE_ND_B48_T16: initCacheInternal_NOCACHE_B48_T16(cacheSize); break;
            case MODE_ND_B48_T32: initCacheInternal_NOCACHE_B48_T32(cacheSize); break;
            case MODE_ND_B64_T32: initCacheInternal_NOCACHE_B64_T32(cacheSize); break;
            case MODE_D_B48_T16:  initCacheInternal_CACHE_DEPTH_B48_T16(cacheSize); break;
            case MODE_D_B48_T32:  initCacheInternal_CACHE_DEPTH_B48_T32(cacheSize); break;
            case MODE_D_B64_T32:  initCacheInternal_CACHE_DEPTH_B64_T32(cacheSize); break;
            default: break;
        }
        resetCacheStats();
    }
}

void renderCacheStats() {
    if (currentMode == MODE_DISABLED || cacheSize == 0) {
        renderOutput("  Cache disabled.", CHEAT_PREFIX);
        return;
    }
    switch (currentMode) {
        case MODE_ND_B48_T16: renderCacheStatsFull_NOCACHE_B48_T16(); break;
        case MODE_ND_B48_T32: renderCacheStatsFull_NOCACHE_B48_T32(); break;
        case MODE_ND_B64_T32: renderCacheStatsFull_NOCACHE_B64_T32(); break;
        case MODE_D_B48_T16:  renderCacheStatsFull_CACHE_DEPTH_B48_T16(); break;
        case MODE_D_B48_T32:  renderCacheStatsFull_CACHE_DEPTH_B48_T32(); break;
        case MODE_D_B64_T32:  renderCacheStatsFull_CACHE_DEPTH_B64_T32(); break;
        default: break;
    }
}
