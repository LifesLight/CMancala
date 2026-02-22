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
#define PREFIX NODEPTH_B48_T16
#define CACHE_DEPTH 0
#define CACHE_B60 0
#define CACHE_T32 0
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B60
#undef CACHE_T32

// 2. NO DEPTH | 48 BIT KEY | 32 BIT TAG
#define PREFIX NODEPTH_B48_T32
#define CACHE_DEPTH 0
#define CACHE_B60 0
#define CACHE_T32 1
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B60
#undef CACHE_T32

// 3. NO DEPTH | 60 BIT KEY | 32 BIT TAG
#define PREFIX NODEPTH_B60_T32
#define CACHE_DEPTH 0
#define CACHE_B60 1
#define CACHE_T32 1
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B60
#undef CACHE_T32

// 4. DEPTH | 48 BIT KEY | 16 BIT TAG
#define PREFIX DEPTH_B48_T16
#define CACHE_DEPTH 1
#define CACHE_B60 0
#define CACHE_T32 0
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B60
#undef CACHE_T32

// 5. DEPTH | 48 BIT KEY | 32 BIT TAG
#define PREFIX DEPTH_B48_T32
#define CACHE_DEPTH 1
#define CACHE_B60 0
#define CACHE_T32 1
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B60
#undef CACHE_T32

// 6. DEPTH | 60 BIT KEY | 32 BIT TAG
#define PREFIX DEPTH_B60_T32
#define CACHE_DEPTH 1
#define CACHE_B60 1
#define CACHE_T32 1
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef CACHE_DEPTH
#undef CACHE_B60
#undef CACHE_T32

// --- Dispatcher ---

typedef enum {
    MODE_DISABLED = -1,
    MODE_ND_B48_T16 = 0,
    MODE_ND_B48_T32,
    MODE_ND_B60_T32,
    MODE_D_B48_T16,
    MODE_D_B48_T32,
    MODE_D_B60_T32,
    MODE_COUNT
} CacheDispatchMode;

// Current Configuration
static CacheDispatchMode currentMode = MODE_DISABLED;
static bool configDepth = true;
static CacheMode configCompressMode = AUTO;
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
        case MODE_ND_B48_T16: freeCacheInternal_NODEPTH_B48_T16(); break;
        case MODE_ND_B48_T32: freeCacheInternal_NODEPTH_B48_T32(); break;
        case MODE_ND_B60_T32: freeCacheInternal_NODEPTH_B60_T32(); break;
        case MODE_D_B48_T16:  freeCacheInternal_DEPTH_B48_T16(); break;
        case MODE_D_B48_T32:  freeCacheInternal_DEPTH_B48_T32(); break;
        case MODE_D_B60_T32:  freeCacheInternal_DEPTH_B60_T32(); break;
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
        cacheSize = 0;
        cacheSizePow = 0;
        return;
    }

    int indexBits = configSizePow - 1;
    bool useCompress = false;

    // Determine Compression Strategy
    if (configCompressMode == ALWAYS_COMPRESS) {
        useCompress = true;
    } else if (configCompressMode == NEVER_COMPRESS) {
        useCompress = false;
    } else {
        // AUTO: Default to T32/B60 (No Compress) for best collision resistance.
        // If the table is too small, a 60-bit key leaves a tag > 32 bits, which is impossible.
        // In that case, we MUST compress.
        if ((60 - indexBits) > 32) {
            useCompress = true;
        } else {
            useCompress = false;
        }
    }

    int keyBits = useCompress ? 48 : 60;

    if (configSizePow >= keyBits) {
        char err[128];
        snprintf(err, sizeof(err), "Fatal: Cache size 2^%d too large for %d-bit keys.", configSizePow, keyBits);
        renderOutput(err, CONFIG_PREFIX);
        quitGame();
        return;
    }

    int tagBitsNeeded = keyBits - indexBits;
    bool useT32 = false;

    if (tagBitsNeeded > 32) {
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
        if (useCompress) {
            useT32 = false;
        } else {
            useT32 = true;
        }
    }

    if (configDepth) {
        if (!useCompress) {
             currentMode = MODE_D_B60_T32;
        } else {
             currentMode = useT32 ? MODE_D_B48_T32 : MODE_D_B48_T16;
        }
    } else {
        if (!useCompress) {
             currentMode = MODE_ND_B60_T32;
        } else {
             currentMode = useT32 ? MODE_ND_B48_T32 : MODE_ND_B48_T16;
        }
    }

    cacheSize = (uint64_t)1 << configSizePow;
    cacheSizePow = configSizePow;

    // Initialize selected mode
    switch (currentMode) {
        case MODE_ND_B48_T16: initCacheInternal_NODEPTH_B48_T16(cacheSize); break;
        case MODE_ND_B48_T32: initCacheInternal_NODEPTH_B48_T32(cacheSize); break;
        case MODE_ND_B60_T32: initCacheInternal_NODEPTH_B60_T32(cacheSize); break;
        case MODE_D_B48_T16:  initCacheInternal_DEPTH_B48_T16(cacheSize); break;
        case MODE_D_B48_T32:  initCacheInternal_DEPTH_B48_T32(cacheSize); break;
        case MODE_D_B60_T32:  initCacheInternal_DEPTH_B60_T32(cacheSize); break;
        default: break;
    }

    resetCacheStats();
}

// --- Public API ---

void setCacheMode(bool depth, CacheMode compressMode) {
    if (getCacheSize() == 0) setCacheSize(DEFAULT_CACHES_SIZE);

    bool sizeChanged = (configSizePow != (int)cacheSizePow);
    bool modeChanged = (configDepth != depth) || (configCompressMode != compressMode);

    configDepth = depth;
    configCompressMode = compressMode;

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

void cacheNodeHash(Board* board, uint64_t boardRep, int evaluation, int boundType, int depth, bool solved) {
    switch (currentMode) {
        case MODE_ND_B48_T16: cacheNodeHash_NODEPTH_B48_T16(board, boardRep, evaluation, boundType, depth, solved); break;
        case MODE_ND_B48_T32: cacheNodeHash_NODEPTH_B48_T32(board, boardRep, evaluation, boundType, depth, solved); break;
        case MODE_ND_B60_T32: cacheNodeHash_NODEPTH_B60_T32(board, boardRep, evaluation, boundType, depth, solved); break;
        case MODE_D_B48_T16:  cacheNodeHash_DEPTH_B48_T16(board, boardRep, evaluation, boundType, depth, solved); break;
        case MODE_D_B48_T32:  cacheNodeHash_DEPTH_B48_T32(board, boardRep, evaluation, boundType, depth, solved); break;
        case MODE_D_B60_T32:  cacheNodeHash_DEPTH_B60_T32(board, boardRep, evaluation, boundType, depth, solved); break;
        default: break;
    }
}

bool getCachedValueHash(Board* board, uint64_t hashValue, int currentDepth, int *evaluation, int *boundType, bool *solved) {
    switch (currentMode) {
        case MODE_ND_B48_T16: return getCachedValueHash_NODEPTH_B48_T16(board, hashValue, currentDepth, evaluation, boundType, solved);
        case MODE_ND_B48_T32: return getCachedValueHash_NODEPTH_B48_T32(board, hashValue, currentDepth, evaluation, boundType, solved);
        case MODE_ND_B60_T32: return getCachedValueHash_NODEPTH_B60_T32(board, hashValue, currentDepth, evaluation, boundType, solved);
        case MODE_D_B48_T16:  return getCachedValueHash_DEPTH_B48_T16(board, hashValue, currentDepth, evaluation, boundType, solved);
        case MODE_D_B48_T32:  return getCachedValueHash_DEPTH_B48_T32(board, hashValue, currentDepth, evaluation, boundType, solved);
        case MODE_D_B60_T32:  return getCachedValueHash_DEPTH_B60_T32(board, hashValue, currentDepth, evaluation, boundType, solved);
        default: return false;
    }
}

uint64_t getCacheSize() {
    if (configSizePow <= 2) return 0;
    return (uint64_t)1 << configSizePow;
}

void stepCache() {
    // Not used right now
}

void invalidateCache() {
    if (currentMode != MODE_DISABLED && cacheSize > 0) {
        switch (currentMode) {
            case MODE_ND_B48_T16: initCacheInternal_NODEPTH_B48_T16(cacheSize); break;
            case MODE_ND_B48_T32: initCacheInternal_NODEPTH_B48_T32(cacheSize); break;
            case MODE_ND_B60_T32: initCacheInternal_NODEPTH_B60_T32(cacheSize); break;
            case MODE_D_B48_T16:  initCacheInternal_DEPTH_B48_T16(cacheSize); break;
            case MODE_D_B48_T32:  initCacheInternal_DEPTH_B48_T32(cacheSize); break;
            case MODE_D_B60_T32:  initCacheInternal_DEPTH_B60_T32(cacheSize); break;
            default: break;
        }
        resetCacheStats();
    }
}

void fillCacheStats(CacheStats* stats, bool calcFrag, bool calcStoneDist, bool calcDepthDist) {
    memset(stats, 0, sizeof(CacheStats));
    if (currentMode == MODE_DISABLED || cacheSize == 0) {
        return;
    }

    switch (currentMode) {
        case MODE_ND_B48_T16: collectCacheStats_NODEPTH_B48_T16(stats, calcFrag, calcStoneDist, calcDepthDist); break;
        case MODE_ND_B48_T32: collectCacheStats_NODEPTH_B48_T32(stats, calcFrag, calcStoneDist, calcDepthDist); break;
        case MODE_ND_B60_T32: collectCacheStats_NODEPTH_B60_T32(stats, calcFrag, calcStoneDist, calcDepthDist); break;
        case MODE_D_B48_T16:  collectCacheStats_DEPTH_B48_T16(stats, calcFrag, calcStoneDist, calcDepthDist); break;
        case MODE_D_B48_T32:  collectCacheStats_DEPTH_B48_T32(stats, calcFrag, calcStoneDist, calcDepthDist); break;
        case MODE_D_B60_T32:  collectCacheStats_DEPTH_B60_T32(stats, calcFrag, calcStoneDist, calcDepthDist); break;
        default: break;
    }
}

void renderCacheStats(bool calcFrag, bool calcStoneDist, bool calcDepthDist) {
    if (currentMode == MODE_DISABLED || cacheSize == 0) {
        renderOutput("  Cache disabled.", CHEAT_PREFIX);
        return;
    }

    CacheStats stats;

    fillCacheStats(&stats, calcFrag, calcStoneDist, calcDepthDist);
    renderCacheOverview(&stats, calcFrag, calcStoneDist, calcDepthDist);
}

bool translateBoard(Board* board, uint64_t *code) {
    switch (currentMode) {
        case MODE_ND_B48_T16: return translateBoard_NODEPTH_B48_T16(board, code);
        case MODE_ND_B48_T32: return translateBoard_NODEPTH_B48_T32(board, code); break;
        case MODE_ND_B60_T32: return translateBoard_NODEPTH_B60_T32(board, code); break;
        case MODE_D_B48_T16:  return translateBoard_DEPTH_B48_T16(  board, code); break;
        case MODE_D_B48_T32:  return translateBoard_DEPTH_B48_T32(  board, code); break;
        case MODE_D_B60_T32:  return translateBoard_DEPTH_B60_T32(  board, code); break;
        default: return false;
    }
}
