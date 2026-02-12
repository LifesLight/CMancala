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
bool isNoDepthMode = false; // Default to DEPTH mode

// --- Performance Tracking ---
uint64_t hits;
uint64_t hitsLegalDepth;
uint64_t sameKeyOverwriteCount;
uint64_t victimOverwriteCount;
uint64_t swapLRUCount;
uint64_t failedEncodeStoneCount;
uint64_t failedEncodeValueRange;

// Snapshot stats for rendering
uint64_t lastHits;
uint64_t lastHitsLegalDepth;
uint64_t lastSameKeyOverwriteCount;
uint64_t lastVictimOverwriteCount;
uint64_t lastSwapLRUCount;
uint64_t lastFailedEncodeStoneCount;
uint64_t lastFailedEncodeValueRange;

// --- Function Pointers for Active Mode ---
typedef void (*CacheNodeFunc)(Board*, int, int, int, bool);
typedef bool (*GetCachedValueFunc)(Board*, int, int*, int*, bool*);

static CacheNodeFunc cacheNodePtr = NULL;
static GetCachedValueFunc getCachedValuePtr = NULL;

// --- Helper Struct for Fragmentation Stats ---
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

// --- Common Shared Logic ---

uint64_t indexHash(uint64_t hash) {
    // Fibonacci Hash, bucket size 2
    return (((hash * 11400714819323198485ULL) >> (65 - cacheSizePow)) * 2);
}

bool translateBoard(Board* board, uint64_t *code) {
    uint64_t h = 0;
    int offset = 0;
    const int a0 = (board->color == 1) ? 0 : 7;
    const int b0 = (board->color == 1) ? 7 : 0;

    for (int i = 0; i < 6; i++) {
        uint8_t v = board->cells[a0 + i];
        if (v > CACHE_MAX_SPC) return false;
        h |= (uint64_t)(v & 0x1F) << offset;
        offset += 5;
    }
    for (int i = 0; i < 6; i++) {
        uint8_t v = board->cells[b0 + i];
        if (v > CACHE_MAX_SPC) return false;
        h |= (uint64_t)(v & 0x1F) << offset;
        offset += 5;
    }
    *code = h;
    return true;
}

// --- Template Instantiation ---

// 1. CACHE_DEPTH (Standard Mode)
#define PREFIX DEPTH
#define HAS_DEPTH 1
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef HAS_DEPTH

// 2. CACHE_NO_DEPTH (Optimized Mode)
#define PREFIX NO_DEPTH
#define HAS_DEPTH 0
#include "logic/solver/impl/cache_template.h"
#undef PREFIX
#undef HAS_DEPTH

// --- Internal Helper to set pointers based on mode ---
static void updateCachePointers() {
    if (isNoDepthMode) {
        cacheNodePtr = cacheNode_NO_DEPTH;
        getCachedValuePtr = getCachedValue_NO_DEPTH;
    } else {
        cacheNodePtr = cacheNode_DEPTH;
        getCachedValuePtr = getCachedValue_DEPTH;
    }
}

// --- Public API ---

void cacheNode(Board* board, int evaluation, int boundType, int depth, bool solved) {
    if (cacheNodePtr) cacheNodePtr(board, evaluation, boundType, depth, solved);
}

bool getCachedValue(Board* board, int currentDepth, int *evaluation, int *boundType, bool *solved) {
    if (getCachedValuePtr) return getCachedValuePtr(board, currentDepth, evaluation, boundType, solved);
    return false;
}

uint64_t getCacheSize() {
    return cacheSize;
}

void stepCache() {
    // No aging logic
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
    // 1. Free existing memory for both potential modes
    freeCacheInternal_DEPTH();
    freeCacheInternal_NO_DEPTH();

    if (sizePow <= 2) {
        cacheSize = 0;
        cacheNodePtr = NULL;
        getCachedValuePtr = NULL;
        resetCacheStats();
        return;
    }

    cacheSize = pow(2, sizePow);
    cacheSizePow = sizePow;

    // 2. Allocate based on current mode
    if (isNoDepthMode) {
        initCacheInternal_NO_DEPTH(cacheSize);
        if (cache_NO_DEPTH == NULL) {
             renderOutput("Failed to allocate NO_DEPTH cache!", CONFIG_PREFIX);
             cacheSize = 0;
        }
    } else {
        initCacheInternal_DEPTH(cacheSize);
        if (cache_DEPTH == NULL) {
             renderOutput("Failed to allocate DEPTH cache!", CONFIG_PREFIX);
             cacheSize = 0;
        }
    }

    // 3. Set pointers
    if (cacheSize > 0) {
        updateCachePointers();
    } else {
        cacheNodePtr = NULL;
        getCachedValuePtr = NULL;
    }

    resetCacheStats();
    // Zero out "Last" stats on restart
    lastHits = 0; lastHitsLegalDepth = 0; lastSameKeyOverwriteCount = 0;
    lastVictimOverwriteCount = 0; lastFailedEncodeStoneCount = 0;
    lastFailedEncodeValueRange = 0; lastSwapLRUCount = 0;
}

void setCacheNoDepth(bool enable) {
    if (isNoDepthMode == enable) return;

    // Free the OLD mode
    if (isNoDepthMode) freeCacheInternal_NO_DEPTH();
    else freeCacheInternal_DEPTH();

    isNoDepthMode = enable;

    // Allocate the NEW mode if cache is active
    if (cacheSize > 0) {
        if (isNoDepthMode) {
            initCacheInternal_NO_DEPTH(cacheSize);
        } else {
            initCacheInternal_DEPTH(cacheSize);
        }
        updateCachePointers();
    }
    
    // Reset stats because we cleared the data
    resetCacheStats();
}

void renderCacheStats() {
    if (cacheSize == 0) {
        renderOutput("  Cache disabled.", CHEAT_PREFIX);
        return;
    }

    // Delegate entirely to the template function to ensure correct output order
    // independent of the function split.
    if (isNoDepthMode) {
        renderCacheStatsFull_NO_DEPTH();
    } else {
        renderCacheStatsFull_DEPTH();
    }
}
