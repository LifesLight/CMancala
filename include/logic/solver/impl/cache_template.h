/**
 * Copyright (c) Alexander Kurtz 2026
 */

#ifndef TOKEN_PASTE
#define TOKEN_PASTE(x, y) x ## _ ## y
#define CAT(x, y) TOKEN_PASTE(x, y)
#endif

#define FN(name) CAT(name, PREFIX)

#if CACHE_T32
#define TAG_TYPE uint32_t
#else
#define TAG_TYPE uint16_t
#endif

// --- Struct Definition ---
typedef struct {
    TAG_TYPE tag_0;

#if CACHE_DEPTH
    uint16_t depth_0;
#endif

    int16_t value_0;

    int16_t value_1;

#if CACHE_DEPTH
    uint16_t depth_1;
#endif

    TAG_TYPE tag_1;

} FN(Bucket);

// --- Unique Global Array ---

static FN(Bucket)* FN(cache) = NULL;

// --- Memory Management Helpers ---

static void FN(freeCacheInternal)() {
    if (FN(cache) != NULL) {
        free(FN(cache));
        FN(cache) = NULL;
    }
}

static void FN(initCacheInternal)(uint64_t size) {
    FN(freeCacheInternal)();
    if (size == 0) return;

    // Allocate half the number of structs, as each holds 2 entries
    uint64_t bucketCount = size >> 1;
    if (bucketCount == 0) bucketCount = 1;

    FN(cache) = malloc(sizeof(FN(Bucket)) * bucketCount);

    if (FN(cache) == NULL) return;

    for (uint64_t i = 0; i < bucketCount; i++) {
        FN(cache)[i].value_0 = CACHE_VAL_UNSET;
        FN(cache)[i].tag_0   = 0;
        FN(cache)[i].value_1 = CACHE_VAL_UNSET;
        FN(cache)[i].tag_1   = 0;

#if CACHE_DEPTH
        FN(cache)[i].depth_0 = 0;
        FN(cache)[i].depth_1 = 0;
#endif
    }
}

// --- Logic ---

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

    // 60-bit mixer (Bijective on 60 bits)
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

    // 48-bit mixer (Bijective on 48 bits)
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
    // Reverse 60-bit mixer
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
    // Reverse 48-bit mixer
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

static inline void FN(splitBoard)(uint64_t boardRep, uint64_t *index, TAG_TYPE *tag) {
    uint64_t bucketMask = (cacheSize >> 1) - 1;
    uint64_t bucketIndex = boardRep & bucketMask;

    *tag = (TAG_TYPE)(boardRep >> (cacheSizePow - 1));
    // Index returned is the Logical Index start (bucketIndex * 2)
    *index = bucketIndex << 1;
}

static inline uint64_t FN(mergeBoard)(uint64_t index, TAG_TYPE tag) {
    // index here is the bucketIndex (which is logical_index >> 1)
    return ((uint64_t)tag << (cacheSizePow - 1)) | index;
}

static inline void FN(cacheNode)(Board* board, int evaluation, int boundType, int depth, bool solved) {
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    evaluation -= scoreDelta;

    if (evaluation > CACHE_VAL_MAX || evaluation < CACHE_VAL_MIN) {
        failedEncodeValueRange++;
        return;
    }

    uint64_t boardRep;
    if (!FN(translateBoard)(board, &boardRep)) {
        failedEncodeStoneCount++;
        return;
    }

    uint64_t index;
    TAG_TYPE tag;
    FN(splitBoard)(boardRep, &index, &tag);

    // Retrieve the bucket (index is logical start, so >> 1 for bucket index)
    FN(Bucket) *b = &FN(cache)[index >> 1];

#if CACHE_DEPTH
    if (solved) {
        depth = DEPTH_SOLVED;
    }
#else
    (void)depth;
    (void)solved;
#endif

    // --- Same-key update ---
    // Slot 0
    if (b->tag_0 == tag) {
#if CACHE_DEPTH
        if (b->depth_0 > depth) return;
        b->depth_0 = depth;
#endif
        b->value_0 = PACK_VALUE(evaluation, boundType);
        sameKeyOverwriteCount++;
        return;
    }
    // Slot 1
    if (b->tag_1 == tag) {
#if CACHE_DEPTH
        if (b->depth_1 > depth) return;
        b->depth_1 = depth;
#endif
        b->value_1 = PACK_VALUE(evaluation, boundType);
        sameKeyOverwriteCount++;
        return;
    }

    // --- Empty slot ---
    if (b->value_0 == CACHE_VAL_UNSET) {
        b->value_0 = PACK_VALUE(evaluation, boundType);
        b->tag_0 = tag;
#if CACHE_DEPTH
        b->depth_0 = depth;
#endif
        return;
    }
    if (b->value_1 == CACHE_VAL_UNSET) {
        b->value_1 = PACK_VALUE(evaluation, boundType);
        b->tag_1 = tag;
#if CACHE_DEPTH
        b->depth_1 = depth;
#endif
        return;
    }

    // --- Victim selection ---
    // 0 = evict slot 0, 1 = evict slot 1
    int victim;

#if CACHE_DEPTH
    if (b->depth_1 != b->depth_0) {
        victim = (b->depth_1 < b->depth_0) ? 1 : 0;
    } else {
        const int zeroExact = (UNPACK_BOUND(b->value_0) == EXACT_BOUND);
        const int oneExact  = (UNPACK_BOUND(b->value_1) == EXACT_BOUND);
        victim = (zeroExact != oneExact) ? (zeroExact ? 1 : 0) : 1;
    }
#else
    const int zeroExact = (UNPACK_BOUND(b->value_0) == EXACT_BOUND);
    const int oneExact  = (UNPACK_BOUND(b->value_1) == EXACT_BOUND);
    victim = (zeroExact != oneExact) ? (zeroExact ? 1 : 0) : 1;
#endif

    victimOverwriteCount++;

    if (victim == 0) {
        b->tag_0 = tag;
        b->value_0 = PACK_VALUE(evaluation, boundType);
#if CACHE_DEPTH
        b->depth_0 = depth;
#endif
    } else {
        b->tag_1 = tag;
        b->value_1 = PACK_VALUE(evaluation, boundType);
#if CACHE_DEPTH
        b->depth_1 = depth;
#endif
    }
}

static inline bool FN(getCachedValue)(Board* board, int currentDepth, int *eval, int *boundType, bool *solved) {
    uint64_t hashValue;
    if (!FN(translateBoard)(board, &hashValue)) return false;

    uint64_t index;
    TAG_TYPE tag;
    FN(splitBoard)(hashValue, &index, &tag);

    FN(Bucket) *b = &FN(cache)[index >> 1];

    // We determine which slot (0 or 1) matches, or -1 if none
    int matchSlot = -1;

    if (b->tag_0 == tag) matchSlot = 0;
    else if (b->tag_1 == tag) matchSlot = 1;
    else return false;

    // Check unset
    int16_t valToCheck = (matchSlot == 0) ? b->value_0 : b->value_1;
    if (valToCheck == CACHE_VAL_UNSET) return false;

    // LRU Swap: If match is in slot 1, swap to slot 0 (MRU)
    if (matchSlot == 1) {
        TAG_TYPE t = b->tag_0; b->tag_0 = b->tag_1; b->tag_1 = t;
        int16_t v = b->value_0; b->value_0 = b->value_1; b->value_1 = v;
#if CACHE_DEPTH
        uint16_t d = b->depth_0; b->depth_0 = b->depth_1; b->depth_1 = d;
#endif
        swapLRUCount++;
    }

    // Now data is in slot 0
    hits++;

#if CACHE_DEPTH
    if (b->depth_0 < currentDepth) return false;
    *solved = (b->depth_0 == DEPTH_SOLVED);
#else
    (void)currentDepth;
    *solved = true; 
#endif

    hitsLegalDepth++;

    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;

    *eval = UNPACK_VALUE(b->value_0) + scoreDelta;
    *boundType = UNPACK_BOUND(b->value_0);

    return true;
}

// --- Stats Collector ---

static void FN(collectCacheStats)(CacheStats* stats) {
    stats->cacheSize = cacheSize;
    // Report size per logical entry (total struct size / 2)
    stats->entrySize = sizeof(FN(Bucket)) / 2;
    stats->setEntries = 0;
    stats->exactCount = 0;
    stats->lowerCount = 0;
    stats->upperCount = 0;

    stats->hasDepth = CACHE_DEPTH;
#if CACHE_DEPTH
    stats->solvedEntries = 0;
    stats->nonSolvedCount = 0;
    stats->depthSum = 0;
    stats->maxDepth = 0;
    memset(stats->depthBins, 0, sizeof(stats->depthBins));
#endif

    // Perf counters from global
    stats->hits = lastHits;
    stats->hitsLegal = lastHitsLegalDepth;
    stats->lruSwaps = lastSwapLRUCount;
    stats->overwriteImprove = lastSameKeyOverwriteCount;
    stats->overwriteEvict = lastVictimOverwriteCount;
    stats->failStones = lastFailedEncodeStoneCount;
    stats->failRange = lastFailedEncodeValueRange;

    // Config String
    const char* depthStr = CACHE_DEPTH ? "Depth" : "No Depth";
    const char* keyStr = CACHE_B60 ? "60-bit Key" : "48-bit Key";
    const char* tagStr = CACHE_T32 ? "32-bit Tag" : "16-bit Tag";
    snprintf(stats->modeStr, sizeof(stats->modeStr), "  Mode:       %s / %s / %s (Packed %zu B)", 
             depthStr, keyStr, tagStr, stats->entrySize);

    // Data Collection for Visualization
    uint64_t sumStones[14] = {0};
    uint64_t countStones[14] = {0};
    uint64_t maxStones[14] = {0};
    uint64_t countOver7[14] = {0};
    uint64_t countOver15[14] = {0};

    // Fragmentation tracking
    int topCount = 0;

    // Check first logical entry (index 0 is bucket 0, slot 0)
    int currentType = (FN(cache)[0].value_0 != CACHE_VAL_UNSET);
    uint64_t chunkStart = 0;
    uint64_t chunkSize2 = 0; // Incremented immediately in loop

    // Iterate Logical entries (0 to cacheSize-1)
    for (uint64_t i = 0; i < cacheSize; i++) {
        uint64_t bucketIdx = i >> 1;
        int slot = i & 1;
        FN(Bucket)* b = &FN(cache)[bucketIdx];

        // Extract logical fields
        int16_t val = (slot == 0) ? b->value_0 : b->value_1;
        TAG_TYPE tag = (slot == 0) ? b->tag_0 : b->tag_1;
#if CACHE_DEPTH
        uint16_t depth = (slot == 0) ? b->depth_0 : b->depth_1;
#endif

        // --- Fragmentation ---
        int t = (val != CACHE_VAL_UNSET);
        if (i == 0) {
            chunkSize2 = 1;
        } else {
             if (t == currentType) {
                 chunkSize2++;
             } else {
                 CacheChunk c = { chunkStart, chunkSize2, currentType };
                 if (topCount < OUTPUT_CHUNK_COUNT) {
                     stats->topChunks[topCount++] = c;
                 } else {
                     int minIdx = 0;
                     for (int k = 1; k < topCount; k++) if (stats->topChunks[k].size < stats->topChunks[minIdx].size) minIdx = k;
                     if (c.size > stats->topChunks[minIdx].size) stats->topChunks[minIdx] = c;
                 }
                 currentType = t;
                 chunkStart = i;
                 chunkSize2 = 1;
             }
        }

        // --- Stats ---
        if (val == CACHE_VAL_UNSET) continue;

        stats->setEntries++;
        int bt = UNPACK_BOUND(val);
        if (bt == EXACT_BOUND) stats->exactCount++;
        else if (bt == LOWER_BOUND) stats->lowerCount++;
        else if (bt == UPPER_BOUND) stats->upperCount++;

#if CACHE_DEPTH
        if (depth == DEPTH_SOLVED) {
            stats->solvedEntries++;
        } else {
            stats->nonSolvedCount++;
            stats->depthSum += depth;
            if (depth > stats->maxDepth) stats->maxDepth = depth;
        }
#endif

        // --- Board Vis ---
        // Reconstruct from bucket index and slot
        uint64_t code = FN(mergeBoard)(bucketIdx, tag);
        Board brd = FN(untranslateBoard)(code);

        for (int k = 0; k < 14; k++) {
            if (k == 6 || k == 13) continue;
            uint8_t stones = brd.cells[k];

            sumStones[k] += stones;
            countStones[k]++;
            if (stones > maxStones[k]) maxStones[k] = stones;

            if (stones > 7) countOver7[k]++;
            if (stones > 15) countOver15[k]++;
        }
    }

    // Final chunk
    CacheChunk c = { chunkStart, chunkSize2, currentType };
    if (topCount < OUTPUT_CHUNK_COUNT) {
        stats->topChunks[topCount++] = c;
    } else {
        int minIdx = 0;
        for (int k = 1; k < topCount; k++) if (stats->topChunks[k].size < stats->topChunks[minIdx].size) minIdx = k;
        if (c.size > stats->topChunks[minIdx].size) stats->topChunks[minIdx] = c;
    }

    // Sort chunks by start index
    qsort(stats->topChunks, topCount, sizeof(CacheChunk), compareChunksStart);
    stats->chunkCount = topCount;

#if CACHE_DEPTH
    if (stats->nonSolvedCount > 0) {
        const uint32_t span = (uint32_t)stats->maxDepth + 1;
        enum { DEPTH_BINS = 8 };
        const uint32_t binW = (span + DEPTH_BINS - 1) / DEPTH_BINS;

        // Pass 2 for histogram
        for (uint64_t i = 0; i < cacheSize; i++) {
            uint64_t bucketIdx = i >> 1;
            int slot = i & 1;
            FN(Bucket)* b = &FN(cache)[bucketIdx];

            int16_t val = (slot == 0) ? b->value_0 : b->value_1;
            if (val == CACHE_VAL_UNSET) continue;

            uint16_t depth = (slot == 0) ? b->depth_0 : b->depth_1;
            if (depth == DEPTH_SOLVED) continue;

            uint32_t bi = depth / binW;
            if (bi >= DEPTH_BINS) bi = DEPTH_BINS - 1;
            stats->depthBins[bi]++;
        }
    }
#endif

    // Calculate Board Vis Aggregates
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
