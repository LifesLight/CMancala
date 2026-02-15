/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

// --- Global State & Stats (Shared by templates) ---
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

// --- Helper: Frag Stats Utils ---
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

// --- Dispatcher Infrastructure ---

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

typedef struct {
    void (*init)(uint64_t);
    void (*freeFunc)(void);
    void (*cacheNode)(Board*, int, int, int, bool);
    bool (*getCached)(Board*, int, int*, int*, bool*);
    bool (*translate)(Board*, uint64_t*);
    void (*renderStats)(void);
} CacheInterface;

// Global Function Pointer Table
static const CacheInterface dispatchTable[MODE_COUNT] = {
    // 0: ND_B48_T16
    { initCacheInternal_NOCACHE_B48_T16, freeCacheInternal_NOCACHE_B48_T16, cacheNode_NOCACHE_B48_T16, getCachedValue_NOCACHE_B48_T16, translateBoard_NOCACHE_B48_T16, renderCacheStatsFull_NOCACHE_B48_T16 },
    // 1: ND_B48_T32
    { initCacheInternal_NOCACHE_B48_T32, freeCacheInternal_NOCACHE_B48_T32, cacheNode_NOCACHE_B48_T32, getCachedValue_NOCACHE_B48_T32, translateBoard_NOCACHE_B48_T32, renderCacheStatsFull_NOCACHE_B48_T32 },
    // 2: ND_B64_T32
    { initCacheInternal_NOCACHE_B64_T32, freeCacheInternal_NOCACHE_B64_T32, cacheNode_NOCACHE_B64_T32, getCachedValue_NOCACHE_B64_T32, translateBoard_NOCACHE_B64_T32, renderCacheStatsFull_NOCACHE_B64_T32 },
    // 3: D_B48_T16
    { initCacheInternal_CACHE_DEPTH_B48_T16, freeCacheInternal_CACHE_DEPTH_B48_T16, cacheNode_CACHE_DEPTH_B48_T16, getCachedValue_CACHE_DEPTH_B48_T16, translateBoard_CACHE_DEPTH_B48_T16, renderCacheStatsFull_CACHE_DEPTH_B48_T16 },
    // 4: D_B48_T32
    { initCacheInternal_CACHE_DEPTH_B48_T32, freeCacheInternal_CACHE_DEPTH_B48_T32, cacheNode_CACHE_DEPTH_B48_T32, getCachedValue_CACHE_DEPTH_B48_T32, translateBoard_CACHE_DEPTH_B48_T32, renderCacheStatsFull_CACHE_DEPTH_B48_T32 },
    // 5: D_B64_T32
    { initCacheInternal_CACHE_DEPTH_B64_T32, freeCacheInternal_CACHE_DEPTH_B64_T32, cacheNode_CACHE_DEPTH_B64_T32, getCachedValue_CACHE_DEPTH_B64_T32, translateBoard_CACHE_DEPTH_B64_T32, renderCacheStatsFull_CACHE_DEPTH_B64_T32 },
};

// Current Configuration
static CacheMode currentMode = MODE_DISABLED;
static bool configDepth = true;     // Default: Depth ON
static bool configCompress = true;  // Default: Compress ON (B48)
static int configSizePow = 0;       // Default: 0

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

/**
 * Calculates the required mode based on configuration and allocates memory.
 * Returns true on success, or quits the game on fatal error.
 */
static void reconfigureCache() {
    // 1. Clean up existing cache
    if (currentMode != MODE_DISABLED) {
        dispatchTable[currentMode].freeFunc();
        currentMode = MODE_DISABLED;
    }

    if (configSizePow <= 2) {
        // Disabled
        cacheSize = 0;
        cacheSizePow = 0;
        return;
    }

    // 2. Logic to determine implementation
    // B64 = !compress
    // B48 = compress
    
    int keyBits = configCompress ? 48 : 64;
    
    // Check if cache is absurdly large (Index bits > Key bits)
    if (configSizePow >= keyBits) {
        char err[128];
        snprintf(err, sizeof(err), "Fatal: Cache size 2^%d too large for %d-bit keys.", configSizePow, keyBits);
        renderOutput(err, CONFIG_PREFIX);
        quitGame();
        return;
    }

    int tagBitsNeeded = keyBits - configSizePow;
    bool useT32 = false;

    // Check bounds
    if (tagBitsNeeded > 32) {
        char err[128];
        snprintf(err, sizeof(err), "Fatal: Cache size 2^%d too small. Tags need %d bits (>32). Enable compression or increase size.", configSizePow, tagBitsNeeded);
        renderOutput(err, CONFIG_PREFIX);
        quitGame();
        return;
    } 
    else if (tagBitsNeeded > 16) {
        useT32 = true;
    } 
    else {
        // Fits in 16 bits, but B64+T16 is explicitly disabled per requirements
        if (!configCompress) {
            useT32 = true;
        } else {
            useT32 = false; // B48 + T16 is allowed
        }
    }

    // 3. Select Enum
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

    dispatchTable[currentMode].init(cacheSize);
    resetCacheStats();
}

// --- Public API ---

void setCacheMode(bool depth, bool compress) {
    if (configDepth == depth && configCompress == compress) return;

    configDepth = depth;
    configCompress = compress;

    // Only reconfigure if we actually have a size set
    if (configSizePow > 0) {
        reconfigureCache();
    }
}

void setCacheSize(int sizePow) {
    if (configSizePow == sizePow) {
        resetCacheStats();
        return;
    }

    configSizePow = sizePow;
}

void cacheNode(Board* board, int evaluation, int boundType, int depth, bool solved) {
    if (currentMode != MODE_DISABLED) {
        dispatchTable[currentMode].cacheNode(board, evaluation, boundType, depth, solved);
    }
}

bool getCachedValue(Board* board, int currentDepth, int *evaluation, int *boundType, bool *solved) {
    if (currentMode != MODE_DISABLED) {
        return dispatchTable[currentMode].getCached(board, currentDepth, evaluation, boundType, solved);
    }
    return false;
}

bool translateBoard(Board* board, uint64_t *code) {
    if (currentMode != MODE_DISABLED) {
        return dispatchTable[currentMode].translate(board, code);
    }
    return false;
}

uint64_t getCacheSize() {
    return cacheSize;
}

void stepCache() {
    // No aging implemented yet
}

void invalidateCache() {
    // Re-init effectively clears it
    if (currentMode != MODE_DISABLED && cacheSize > 0) {
        dispatchTable[currentMode].init(cacheSize);
        resetCacheStats();
    }
}

void renderCacheStats() {
    if (currentMode == MODE_DISABLED || cacheSize == 0) {
        renderOutput("  Cache disabled.", CHEAT_PREFIX);
        return;
    }
    dispatchTable[currentMode].renderStats();
}
