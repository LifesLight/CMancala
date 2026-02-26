/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/impl/macros.h"
#include <stdatomic.h>

#if CACHE_T32
#define TAG_TYPE uint32_t
#else
#define TAG_TYPE uint16_t
#endif

// --- Struct Definition ---
typedef struct {
    _Atomic TAG_TYPE tag_0;
#if CACHE_DEPTH
    _Atomic uint16_t depth_0;
#endif
    _Atomic int16_t value_0;

    _Atomic int16_t value_1;
#if CACHE_DEPTH
    _Atomic uint16_t depth_1;
#endif
    _Atomic TAG_TYPE tag_1;
} FN(Bucket);

static FN(Bucket)* FN(cache) = NULL;

static void FN(freeCacheInternal)() {
    if (FN(cache) != NULL) {
        free(FN(cache));
        FN(cache) = NULL;
    }
}

static void FN(initCacheInternal)(uint64_t size) {
    FN(freeCacheInternal)();
    if (size == 0) return;

    uint64_t bucketCount = size >> 1;
    if (bucketCount == 0) bucketCount = 1;

    FN(cache) = malloc(sizeof(FN(Bucket)) * bucketCount);
    if (FN(cache) == NULL) return;

    for (uint64_t i = 0; i < bucketCount; i++) {
        atomic_init(&FN(cache)[i].value_0, CACHE_VAL_UNSET);
        atomic_init(&FN(cache)[i].tag_0, 0);
        atomic_init(&FN(cache)[i].value_1, CACHE_VAL_UNSET);
        atomic_init(&FN(cache)[i].tag_1, 0);

#if CACHE_DEPTH
        atomic_init(&FN(cache)[i].depth_0, 0);
        atomic_init(&FN(cache)[i].depth_1, 0);
#endif
    }
}

static inline bool FN(translateBoard)(Board* board, uint64_t *code) {
    uint64_t h = 0;
    int offset = 0;

    const int a0 = (board->color == 1) ? 0 : 7;
    const int b0 = (board->color == 1) ? 7 : 0;

#if CACHE_B60
    for (int i = 0; i < 6; i++) {
        uint8_t v = board->cells[a0 + i];
        if (v > 31) return false;
        h |= (uint64_t)(v & 0x1F) << offset;
        offset += 5;
    }
    for (int i = 0; i < 6; i++) {
        uint8_t v = board->cells[b0 + i];
        if (v > 31) return false;
        h |= (uint64_t)(v & 0x1F) << offset;
        offset += 5;
    }

    const uint64_t m = 0x0FFFFFFFFFFFFFFFULL;
    h ^= h >> 30;
    h = (h * 0xff51afd7ed558ccdULL) & m;
    h ^= h >> 30;
    h = (h * 0xc4ceb9fe1a85ec53ULL) & m;
    h ^= h >> 30;
#else
    for (int i = 0; i < 6; i++) {
        uint8_t v = board->cells[a0 + i];
        if (v > 15) return false;
        h |= (uint64_t)(v & 0x0F) << offset;
        offset += 4;
    }
    for (int i = 0; i < 6; i++) {
        uint8_t v = board->cells[b0 + i];
        if (v > 15) return false;
        h |= (uint64_t)(v & 0x0F) << offset;
        offset += 4;
    }

    const uint64_t m = 0xFFFFFFFFFFFFULL;
    h ^= h >> 24;
    h = (h * 0xfd7ed558ccdULL) & m;
    h ^= h >> 24;
    h = (h * 0xfe1a85ec53ULL) & m;
    h ^= h >> 24;
#endif

    *code = h;
    return true;
}

static inline Board FN(untranslateBoard)(uint64_t code) {
    Board board = {0};
    board.color = 1;
    uint64_t h = code;
    int offset = 0;

#if CACHE_B60
    const uint64_t m = 0x0FFFFFFFFFFFFFFFULL;
    h ^= h >> 30;
    h = (h * 0x0cb4b2f8129337dbULL) & m;
    h ^= h >> 30;
    h = (h * 0x0f74430c22a54005ULL) & m;
    h ^= h >> 30;

    for (int i = 0; i < 6; i++) {
        board.cells[i] = (uint8_t)((h >> offset) & 0x1F);
        offset += 5;
    }
    for (int i = 0; i < 6; i++) {
        board.cells[7 + i] = (uint8_t)((h >> offset) & 0x1F);
        offset += 5;
    }
#else
    const uint64_t m = 0xFFFFFFFFFFFFULL;
    h ^= h >> 24;
    h = (h * 0x3f8129337dbULL) & m;
    h ^= h >> 24;
    h = (h * 0xe30c22a54005ULL) & m;
    h ^= h >> 24;

    for (int i = 0; i < 6; i++) {
        board.cells[i] = (uint8_t)((h >> offset) & 0x0F);
        offset += 4;
    }
    for (int i = 0; i < 6; i++) {
        board.cells[7 + i] = (uint8_t)((h >> offset) & 0x0F);
        offset += 4;
    }
#endif
    return board;
}

static inline void FN(splitBoard)(uint64_t boardRep, uint64_t *bucketIndex, TAG_TYPE *tag) {
    uint64_t bucketMask = (cacheSize >> 1) - 1;
    *bucketIndex = boardRep & bucketMask;
    *tag = (TAG_TYPE)(boardRep >> (cacheSizePow - 1));
}

static inline uint64_t FN(mergeBoard)(uint64_t bucketIndex, TAG_TYPE tag) {
    return ((uint64_t)tag << (cacheSizePow - 1)) | bucketIndex;
}

static inline void FN(cacheNodeHash)(Board* board, uint64_t boardRep, int evaluation, int boundType, int depth, bool solved) {
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    evaluation -= scoreDelta;

    if (evaluation > CACHE_VAL_MAX || evaluation < CACHE_VAL_MIN) {
        failedEncodeValueRange++;
        return;
    }

    uint64_t index;
    TAG_TYPE tag;
    FN(splitBoard)(boardRep, &index, &tag);

    FN(Bucket) *b = &FN(cache)[index];

#if CACHE_DEPTH
    if (solved) depth = DEPTH_SOLVED;
#else
    (void)depth;
    (void)solved;
#endif

    TAG_TYPE t0 = atomic_load_explicit(&b->tag_0, memory_order_relaxed);
    if (t0 == tag) {
#if CACHE_DEPTH
        uint16_t d0 = atomic_load_explicit(&b->depth_0, memory_order_relaxed);
        if (d0 > depth) return;
        atomic_store_explicit(&b->depth_0, depth, memory_order_relaxed);
#endif
        atomic_store_explicit(&b->value_0, PACK_VALUE(evaluation, boundType), memory_order_relaxed);
        atomic_store_explicit(&b->tag_0, tag, memory_order_release);
        sameKeyOverwriteCount++;
        return;
    }
    
    TAG_TYPE t1 = atomic_load_explicit(&b->tag_1, memory_order_relaxed);
    if (t1 == tag) {
#if CACHE_DEPTH
        uint16_t d1 = atomic_load_explicit(&b->depth_1, memory_order_relaxed);
        if (d1 > depth) return;
        atomic_store_explicit(&b->depth_1, depth, memory_order_relaxed);
#endif
        atomic_store_explicit(&b->value_1, PACK_VALUE(evaluation, boundType), memory_order_relaxed);
        atomic_store_explicit(&b->tag_1, tag, memory_order_release);
        sameKeyOverwriteCount++;
        return;
    }

    int16_t v0 = atomic_load_explicit(&b->value_0, memory_order_relaxed);
    if (v0 == CACHE_VAL_UNSET) {
#if CACHE_DEPTH
        atomic_store_explicit(&b->depth_0, depth, memory_order_relaxed);
#endif
        atomic_store_explicit(&b->value_0, PACK_VALUE(evaluation, boundType), memory_order_relaxed);
        atomic_store_explicit(&b->tag_0, tag, memory_order_release);
        return;
    }
    
    int16_t v1 = atomic_load_explicit(&b->value_1, memory_order_relaxed);
    if (v1 == CACHE_VAL_UNSET) {
#if CACHE_DEPTH
        atomic_store_explicit(&b->depth_1, depth, memory_order_relaxed);
#endif
        atomic_store_explicit(&b->value_1, PACK_VALUE(evaluation, boundType), memory_order_relaxed);
        atomic_store_explicit(&b->tag_1, tag, memory_order_release);
        return;
    }

    int victim;
    victimOverwriteCount++;

#if CACHE_DEPTH
    uint16_t d0 = atomic_load_explicit(&b->depth_0, memory_order_relaxed);
    uint16_t d1 = atomic_load_explicit(&b->depth_1, memory_order_relaxed);
    if (d1 != d0) {
        victim = (d1 < d0) ? 1 : 0;
    } else {
        const int zeroExact = (UNPACK_BOUND(v0) == EXACT_BOUND);
        const int oneExact  = (UNPACK_BOUND(v1) == EXACT_BOUND);
        victim = (zeroExact != oneExact) ? (zeroExact ? 1 : 0) : 1;
    }
#else
    const int zeroExact = (UNPACK_BOUND(v0) == EXACT_BOUND);
    const int oneExact  = (UNPACK_BOUND(v1) == EXACT_BOUND);
    victim = (zeroExact != oneExact) ? (zeroExact ? 1 : 0) : 1;
#endif

    if (victim == 0) {
#if CACHE_DEPTH
        atomic_store_explicit(&b->depth_0, depth, memory_order_relaxed);
#endif
        atomic_store_explicit(&b->value_0, PACK_VALUE(evaluation, boundType), memory_order_relaxed);
        atomic_store_explicit(&b->tag_0, tag, memory_order_release);
    } else {
#if CACHE_DEPTH
        atomic_store_explicit(&b->depth_1, depth, memory_order_relaxed);
#endif
        atomic_store_explicit(&b->value_1, PACK_VALUE(evaluation, boundType), memory_order_relaxed);
        atomic_store_explicit(&b->tag_1, tag, memory_order_release);
    }
}

static inline bool FN(getCachedValueHash)(Board* board, uint64_t hashValue, int currentDepth, int *eval, int *boundType, bool *solved) {
    uint64_t index;
    TAG_TYPE tag;
    FN(splitBoard)(hashValue, &index, &tag);

    FN(Bucket) *b = &FN(cache)[index];

    TAG_TYPE t0_first = atomic_load_explicit(&b->tag_0, memory_order_acquire);
    TAG_TYPE t1_first = atomic_load_explicit(&b->tag_1, memory_order_acquire);

    int matchSlot = -1;
    if (t0_first == tag) matchSlot = 0;
    else if (t1_first == tag) matchSlot = 1;
    else return false;

    int16_t val;
#if CACHE_DEPTH
    uint16_t dep;
#endif

    if (matchSlot == 0) {
        val = atomic_load_explicit(&b->value_0, memory_order_relaxed);
#if CACHE_DEPTH
        dep = atomic_load_explicit(&b->depth_0, memory_order_relaxed);
#endif
        if (atomic_load_explicit(&b->tag_0, memory_order_acquire) != t0_first) return false;
    } else {
        val = atomic_load_explicit(&b->value_1, memory_order_relaxed);
#if CACHE_DEPTH
        dep = atomic_load_explicit(&b->depth_1, memory_order_relaxed);
#endif
        if (atomic_load_explicit(&b->tag_1, memory_order_acquire) != t1_first) return false;
    }

    if (val == CACHE_VAL_UNSET) return false;

    if (matchSlot == 1) {
        TAG_TYPE st0 = atomic_load_explicit(&b->tag_0, memory_order_relaxed);
        TAG_TYPE st1 = atomic_load_explicit(&b->tag_1, memory_order_relaxed);
        int16_t sv0 = atomic_load_explicit(&b->value_0, memory_order_relaxed);
        int16_t sv1 = atomic_load_explicit(&b->value_1, memory_order_relaxed);
        
        atomic_store_explicit(&b->tag_0, st1, memory_order_relaxed);
        atomic_store_explicit(&b->tag_1, st0, memory_order_relaxed);
        atomic_store_explicit(&b->value_0, sv1, memory_order_relaxed);
        atomic_store_explicit(&b->value_1, sv0, memory_order_relaxed);

#if CACHE_DEPTH
        uint16_t sd0 = atomic_load_explicit(&b->depth_0, memory_order_relaxed);
        uint16_t sd1 = atomic_load_explicit(&b->depth_1, memory_order_relaxed);
        atomic_store_explicit(&b->depth_0, sd1, memory_order_relaxed);
        atomic_store_explicit(&b->depth_1, sd0, memory_order_relaxed);
#endif
        swapLRUCount++;
    }

    hits++;

#if CACHE_DEPTH
    if (dep < currentDepth) return false;
    *solved = (dep == DEPTH_SOLVED);
#else
    (void)currentDepth;
    *solved = true; 
#endif

    hitsLegalDepth++;

    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;

    *eval = UNPACK_VALUE(val) + scoreDelta;
    *boundType = UNPACK_BOUND(val);

    return true;
}

static void FN(collectCacheStats)(CacheStats* stats, bool calcFrag, bool calcStoneDist, bool calcDepthDist) {
    stats->cacheSize = cacheSize;
    stats->entrySize = sizeof(FN(Bucket)) / 2;

    uint64_t setEntries = 0;
    uint64_t exactCount = 0;
    uint64_t lowerCount = 0;
    uint64_t upperCount = 0;

#if CACHE_DEPTH
    uint64_t solvedEntries = 0;
    uint64_t nonSolvedCount = 0;
    uint64_t depthSum = 0;
    uint16_t maxDepth = 0;
    memset(stats->depthBins, 0, sizeof(stats->depthBins));
#else
    (void)calcDepthDist;
#endif

    stats->hasDepth = CACHE_DEPTH;

    stats->hits = lastHits;
    stats->hitsLegal = lastHitsLegalDepth;
    stats->lruSwaps = lastSwapLRUCount;
    stats->overwriteImprove = lastSameKeyOverwriteCount;
    stats->overwriteEvict = lastVictimOverwriteCount;
    stats->failStones = lastFailedEncodeStoneCount;
    stats->failRange = lastFailedEncodeValueRange;

    const char* depthStr = CACHE_DEPTH ? "Depth" : "No Depth";
    const char* keyStr = CACHE_B60 ? "60-bit Key" : "48-bit Key";
    const char* tagStr = CACHE_T32 ? "32-bit Tag" : "16-bit Tag";
    snprintf(stats->modeStr, sizeof(stats->modeStr), "  Mode:       %s / %s / %s (%zu Bytes)", depthStr, keyStr, tagStr, stats->entrySize);

    uint64_t sumStones[14] = {0};
    uint64_t countStones[14] = {0};
    uint64_t maxStones[14] = {0};
    uint64_t countOver7[14] = {0};
    uint64_t countOver15[14] = {0};

    int topCount = 0;
    uint64_t chunkStart = 0;
    uint64_t chunkSize = 0; 

    int currentType = (atomic_load_explicit(&FN(cache)[0].value_0, memory_order_relaxed) != CACHE_VAL_UNSET);

    uint64_t bucketCount = cacheSize >> 1;

    for (uint64_t i = 0; i < bucketCount; i++) {
        FN(Bucket)* b = &FN(cache)[i];
        for (int slot = 0; slot < 2; slot++) {
            int16_t val = (slot == 0) ? atomic_load_explicit(&b->value_0, memory_order_relaxed) : atomic_load_explicit(&b->value_1, memory_order_relaxed);
            int type = (val != CACHE_VAL_UNSET);

            if (calcFrag) {
                if (i == 0 && slot == 0) {
                    chunkSize = 1;
                } else {
                    if (type == currentType) {
                        chunkSize++;
                    } else {
                        CacheChunk c = { chunkStart, chunkSize, currentType };
                        if (topCount < OUTPUT_CHUNK_COUNT) {
                            stats->topChunks[topCount++] = c;
                        } else {
                            int minIdx = 0;
                            for (int k = 1; k < topCount; k++) if (stats->topChunks[k].size < stats->topChunks[minIdx].size) minIdx = k;
                            if (c.size > stats->topChunks[minIdx].size) stats->topChunks[minIdx] = c;
                        }
                        currentType = type;
                        chunkStart = (i << 1) + slot;
                        chunkSize = 1;
                    }
                }
            }

            if (!type) continue;
            setEntries++;
            int bt = UNPACK_BOUND(val);
            if (bt == EXACT_BOUND) exactCount++;
            else if (bt == LOWER_BOUND) lowerCount++;
            else upperCount++;

#if CACHE_DEPTH
            uint16_t d = (slot == 0) ? atomic_load_explicit(&b->depth_0, memory_order_relaxed) : atomic_load_explicit(&b->depth_1, memory_order_relaxed);
            if (d == DEPTH_SOLVED) {
                solvedEntries++;
            } else {
                nonSolvedCount++;
                depthSum += d;
                if (d > maxDepth) maxDepth = d;
            }
#endif

            if (calcStoneDist) {
                TAG_TYPE tag = (slot == 0) ? atomic_load_explicit(&b->tag_0, memory_order_relaxed) : atomic_load_explicit(&b->tag_1, memory_order_relaxed);
                uint64_t code = FN(mergeBoard)(i, tag);
                Board brd = FN(untranslateBoard)(code);

                for (int k = 0; k < 14; k++) {
                    if (k == 6 || k == 13) continue;
                    uint8_t s = brd.cells[k];
                    sumStones[k] += s;
                    countStones[k]++;
                    if (s > maxStones[k]) maxStones[k] = s;
                    if (s > 7) countOver7[k]++;
                    if (s > 15) countOver15[k]++;
                }
            }
        }
    }

    stats->setEntries = setEntries;
    stats->exactCount = exactCount;
    stats->lowerCount = lowerCount;
    stats->upperCount = upperCount;

#if CACHE_DEPTH
    stats->solvedEntries = solvedEntries;
    stats->nonSolvedCount = nonSolvedCount;
    stats->depthSum = depthSum;
    stats->maxDepth = maxDepth;
#endif

    if (calcFrag) {
        CacheChunk c = { chunkStart, chunkSize, currentType };
        if (topCount < OUTPUT_CHUNK_COUNT) {
            stats->topChunks[topCount++] = c;
        } else {
            int minIdx = 0;
            for (int k = 1; k < topCount; k++) if (stats->topChunks[k].size < stats->topChunks[minIdx].size) minIdx = k;
            if (c.size > stats->topChunks[minIdx].size) stats->topChunks[minIdx] = c;
        }
        qsort(stats->topChunks, topCount, sizeof(CacheChunk), compareChunksStart);
        stats->chunkCount = topCount;
    } else {
        stats->chunkCount = 0;
    }

#if CACHE_DEPTH
    if (calcDepthDist && nonSolvedCount > 0) {
        const uint32_t span = (uint32_t)maxDepth + 1;
        enum { DEPTH_BINS = 8 };
        const uint32_t binW = (span + DEPTH_BINS - 1) / DEPTH_BINS;

        for (uint64_t i = 0; i < bucketCount; i++) {
            FN(Bucket)* b = &FN(cache)[i];
            for (int slot = 0; slot < 2; slot++) {
                int16_t val = (slot == 0) ? atomic_load_explicit(&b->value_0, memory_order_relaxed) : atomic_load_explicit(&b->value_1, memory_order_relaxed);
                if (val == CACHE_VAL_UNSET) continue;
                uint16_t d = (slot == 0) ? atomic_load_explicit(&b->depth_0, memory_order_relaxed) : atomic_load_explicit(&b->depth_1, memory_order_relaxed);
                if (d == DEPTH_SOLVED) continue;
                uint32_t bi = d / binW;
                if (bi >= DEPTH_BINS) bi = DEPTH_BINS - 1;
                stats->depthBins[bi]++;
            }
        }
    }
#endif

    for (int k = 0; k < 14; k++) {
        if (k == 6 || k == 13) {
            stats->avgStones[k] = 0;
            stats->maxStones[k] = 0;
            stats->over7[k] = 0;
            stats->over15[k] = 0;
            continue;
        }
        if (countStones[k] > 0) {
            stats->avgStones[k] = (double)sumStones[k] / (double)countStones[k];
            stats->maxStones[k] = (double)maxStones[k];
            stats->over7[k]  = (countOver7[k] > 0)  ? log10((double)countOver7[k])  : 0.0;
            stats->over15[k] = (countOver15[k] > 0) ? log10((double)countOver15[k]) : 0.0;
        } else {
            stats->avgStones[k] = 0;
            stats->maxStones[k] = 0;
            stats->over7[k] = 0;
            stats->over15[k] = 0;
        }
    }
}
#undef TAG_TYPE
