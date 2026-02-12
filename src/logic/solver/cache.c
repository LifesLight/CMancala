/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// --- Global State ---
uint64_t cacheSize = 0;
uint32_t cacheSizePow = 0;
bool isNoDepthMode = false;

// --- Performance Tracking ---
uint64_t hits;
uint64_t hitsLegalDepth;
uint64_t sameKeyOverwriteCount;
uint64_t victimOverwriteCount;
uint64_t swapLRUCount;
uint64_t failedEncodeStoneCount;
uint64_t failedEncodeValueRange;

// Snapshot stats
uint64_t lastHits;
uint64_t lastHitsLegalDepth;
uint64_t lastSameKeyOverwriteCount;
uint64_t lastVictimOverwriteCount;
uint64_t lastSwapLRUCount;
uint64_t lastFailedEncodeStoneCount;
uint64_t lastFailedEncodeValueRange;

// --- Common Helper: Frag Stats ---
typedef struct {
    uint64_t start;
    uint64_t size;
    int type;
} Chunk;

int compareChunksSize(const void *a, const void *b) {
    return ((Chunk*)b)->size - ((Chunk*)a)->size;
}

int compareChunksStart(const void *a, const void *b) {
    return ((Chunk*)a)->start - ((Chunk*)b)->start;
}

// --- Common Logic ---

uint64_t indexHash(uint64_t hash) {
    // Fibonacci Hash, bucket size 2
    return (((hash * 11400714819323198485ULL) >> (65 - cacheSizePow)) * 2);
}

// --- Template Instantiation ---

#define PREFIX DEPTH
#define HAS_DEPTH 1
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef HAS_DEPTH

#define PREFIX NO_DEPTH
#define HAS_DEPTH 0
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef HAS_DEPTH

// --- Public API ---

void cacheNode(Board* board, int evaluation, int boundType, int depth, bool solved) {
    if (isNoDepthMode) {
        cacheNode_NO_DEPTH(board, evaluation, boundType, depth, solved);
    } else {
        cacheNode_DEPTH(board, evaluation, boundType, depth, solved);
    }
}

bool getCachedValue(Board* board, int currentDepth, int *evaluation, int *boundType, bool *solved) {
    if (isNoDepthMode) {
        return getCachedValue_NO_DEPTH(board, currentDepth, evaluation, boundType, solved);
    } else {
        return getCachedValue_DEPTH(board, currentDepth, evaluation, boundType, solved);
    }
}

bool translateBoard(Board* board, uint64_t *code) {
    if (isNoDepthMode) {
        return translateBoard_NO_DEPTH(board, code);
    } else {
        return translateBoard_DEPTH(board, code);
    }
}

uint64_t getCacheSize() {
    return cacheSize;
}

void stepCache() {
    // No aging
}

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

void startCache(int sizePow) {
    uint64_t targetSize;
    uint32_t targetPow;

    if (sizePow <= 2) {
        targetSize = 0;
        targetPow = 0;
    } else {
        targetSize = pow(2, sizePow);
        targetPow = sizePow;
    }

    if (targetSize == cacheSize) {
        resetCacheStats();
        return;
    }

    freeCacheInternal_DEPTH();
    freeCacheInternal_NO_DEPTH();

    cacheSize = targetSize;
    cacheSizePow = targetPow;

    resetCacheStats();
    lastHits = 0; lastHitsLegalDepth = 0; lastSameKeyOverwriteCount = 0;
    lastVictimOverwriteCount = 0; lastFailedEncodeStoneCount = 0;
    lastFailedEncodeValueRange = 0; lastSwapLRUCount = 0;
}

void setCacheNoDepth(bool enable) {
    // 1. Check if we are already in the correct state and allocated
    if (enable) {
        if (isNoDepthMode && cache_NO_DEPTH != NULL) return;
    } else {
        if (!isNoDepthMode && cache_DEPTH != NULL) return;
    }

    if (isNoDepthMode) {
        freeCacheInternal_NO_DEPTH();
    } else {
        freeCacheInternal_DEPTH();
    }

    isNoDepthMode = enable;

    // 4. Allocate new mode
    if (cacheSize > 0) {
        if (isNoDepthMode) {
            initCacheInternal_NO_DEPTH(cacheSize);
            if (cache_NO_DEPTH == NULL) {
                 renderOutput("Fatal: Failed to allocate NO_DEPTH cache!", CONFIG_PREFIX);
                 quitGame();
            }
        } else {
            initCacheInternal_DEPTH(cacheSize);
            if (cache_DEPTH == NULL) {
                 renderOutput("Fatal: Failed to allocate DEPTH cache!", CONFIG_PREFIX);
                 quitGame();
            }
        }
    }

    resetCacheStats();
}

void renderCacheStats() {
    if (cacheSize == 0) {
        renderOutput("  Cache disabled.", CHEAT_PREFIX);
        return;
    }

    if (isNoDepthMode) {
        renderCacheStatsFull_NO_DEPTH();
    } else {
        renderCacheStatsFull_DEPTH();
    }
}
