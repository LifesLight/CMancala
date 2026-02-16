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
    TAG_TYPE tag;

#if CACHE_DEPTH
    uint16_t depth;
#endif

    int16_t value;
} FN(Entry);

// --- Unique Global Array ---
static FN(Entry)* FN(cache) = NULL;

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

    FN(cache) = malloc(sizeof(FN(Entry)) * size);

    if (FN(cache) == NULL) return;

    for (uint64_t i = 0; i < size; i++) {
        FN(cache)[i].value = CACHE_VAL_UNSET;
        FN(cache)[i].tag = 0;

#if CACHE_DEPTH
        FN(cache)[i].depth = 0;
#endif
    }
}

// --- Logic ---

static inline bool FN(translateBoard)(Board* board, uint64_t *code) {
    uint64_t h = 0;
    int offset = 0;

    const int a0 = (board->color == 1) ? 0 : 7;
    const int b0 = (board->color == 1) ? 7 : 0;

#if CACHE_B64
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

    // MurmurHash3-style 64-bit mixer (Bijective)
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;

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

#if CACHE_B64
    // Reverse 64-bit mixer
    h ^= h >> 33;
    h *= 0x9cb4b2f8129337dbULL;
    h ^= h >> 33;
    h *= 0x4f74430c22a54005ULL;
    h ^= h >> 33;

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
    *index = bucketIndex << 1;
}

static inline uint64_t FN(mergeBoard)(uint64_t index, TAG_TYPE tag) {
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

    // index points to the start of the bucket (2 entries)
    FN(Entry) *b = &FN(cache)[index];

#if CACHE_DEPTH
    if (solved) {
        depth = DEPTH_SOLVED;
    }
#else
    (void)depth;
    (void)solved;
#endif

    // Same-key update (check both slots in the bucket)
    for (int i = 0; i < 2; i++) {
        if (b[i].tag == tag) {
#if CACHE_DEPTH
// TODO: Maybe check for exact bounds here
            if (b[i].depth > depth) return;
            b[i].depth = depth;
#endif
            b[i].value = PACK_VALUE(evaluation, boundType);
            sameKeyOverwriteCount++;
            return;
        }
    }

    // Empty slot
    for (int i = 0; i < 2; i++) {
        if (b[i].value == CACHE_VAL_UNSET) {
            b[i].value = PACK_VALUE(evaluation, boundType);
            b[i].tag = tag;
#if CACHE_DEPTH
            b[i].depth = depth;
#endif
            return;
        }
    }

    // Victim selection
    int victim;

    // With depth, prioritize higher depth entries
#if CACHE_DEPTH
    if (b[1].depth != b[0].depth) {
        victim = (b[1].depth < b[0].depth) ? 1 : 0;
    } else {
        const int zeroExact = (UNPACK_BOUND(b[0].value) == EXACT_BOUND);
        const int oneExact  = (UNPACK_BOUND(b[1].value) == EXACT_BOUND);
        victim = (zeroExact != oneExact) ? (zeroExact ? 1 : 0) : 1;
    }
#else
    const int zeroExact = (UNPACK_BOUND(b[0].value) == EXACT_BOUND);
    const int oneExact  = (UNPACK_BOUND(b[1].value) == EXACT_BOUND);
    victim = (zeroExact != oneExact) ? (zeroExact ? 1 : 0) : 1;
#endif

    victimOverwriteCount++;
    b[victim].tag = tag;
    b[victim].value = PACK_VALUE(evaluation, boundType);

#if CACHE_DEPTH
    b[victim].depth = depth;
#endif
}

static inline bool FN(getCachedValue)(Board* board, int currentDepth, int *eval, int *boundType, bool *solved) {
    uint64_t hashValue;
    if (!FN(translateBoard)(board, &hashValue)) return false;

    uint64_t index;
    TAG_TYPE tag;
    FN(splitBoard)(hashValue, &index, &tag);

    // index points to start of bucket
    FN(Entry) *entryBase = &FN(cache)[index];
    FN(Entry) *e = NULL;

    if (entryBase[0].tag == tag) {
        e = &entryBase[0];
    } else if (entryBase[1].tag == tag) {
        // LRU Swap: Move slot 1 to slot 0 (MRU)
        FN(Entry) tmp = entryBase[0];
        entryBase[0] = entryBase[1];
        entryBase[1] = tmp;
        e = &entryBase[0];
        swapLRUCount++;
    } else {
        return false;
    }

    if (e->value == CACHE_VAL_UNSET) {
        return false;
    }

    hits++;

#if CACHE_DEPTH
    if (e->depth < currentDepth) return false;
    *solved = (e->depth == DEPTH_SOLVED);
#else
    (void)currentDepth;
    *solved = true; 
#endif

    hitsLegalDepth++;

    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;

    *eval = UNPACK_VALUE(e->value) + scoreDelta;
    *boundType = UNPACK_BOUND(e->value);

    return true;
}

// --- Stats Collector ---

static void FN(collectCacheStats)(CacheStats* stats) {
    stats->cacheSize = cacheSize;
    stats->entrySize = sizeof(FN(Entry));
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
    const char* keyStr = CACHE_B64 ? "64-bit Key" : "48-bit Key";
    const char* tagStr = CACHE_T32 ? "32-bit Tag" : "16-bit Tag";
    snprintf(stats->modeStr, sizeof(stats->modeStr), "  Mode:       %s / %s / %s (%zu Bytes)", 
             depthStr, keyStr, tagStr, sizeof(FN(Entry)));

    // Data Collection for Visualization
    uint64_t hist[14][32]; 
    memset(hist, 0, sizeof(hist));
    
#if CACHE_B64
    const int cap = 31;
#else
    const int cap = 15;
#endif
    stats->riskThreshold = (int)(cap * 0.80);

    // Fragmentation tracking
    int topCount = 0;
    int currentType = (FN(cache)[0].value != CACHE_VAL_UNSET);
    uint64_t chunkStart = 0;
    uint64_t chunkSize2 = 1;

    for (uint64_t i = 0; i < cacheSize; i++) {
        // --- Fragmentation ---
        if (i > 0) {
             const int t = (FN(cache)[i].value != CACHE_VAL_UNSET);
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
        if (FN(cache)[i].value == CACHE_VAL_UNSET) continue;
        
        stats->setEntries++;
        int bt = UNPACK_BOUND(FN(cache)[i].value);
        if (bt == EXACT_BOUND) stats->exactCount++;
        else if (bt == LOWER_BOUND) stats->lowerCount++;
        else if (bt == UPPER_BOUND) stats->upperCount++;

#if CACHE_DEPTH
        if (FN(cache)[i].depth == DEPTH_SOLVED) {
            stats->solvedEntries++;
        } else {
            stats->nonSolvedCount++;
            stats->depthSum += FN(cache)[i].depth;
            if (FN(cache)[i].depth > stats->maxDepth) stats->maxDepth = FN(cache)[i].depth;
        }
#endif

        // --- Board Vis ---
        TAG_TYPE tag = FN(cache)[i].tag;
        // Reconstruct from bucket index
        uint64_t bucketIndex = i >> 1;
        uint64_t code = FN(mergeBoard)(bucketIndex, tag);
        Board b = FN(untranslateBoard)(code);

        for (int k = 0; k < 14; k++) {
            if (k == 6 || k == 13) continue;
            uint8_t stones = b.cells[k];
            if (stones > cap) stones = cap;
            hist[k][stones]++;
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
        // Bin calculation depends on maxDepth found
        const uint32_t span = (uint32_t)stats->maxDepth + 1;
        enum { DEPTH_BINS = 8 };
        const uint32_t binW = (span + DEPTH_BINS - 1) / DEPTH_BINS;
        
        // Pass 2 for histogram
        for (uint64_t i = 0; i < cacheSize; i++) {
            if (FN(cache)[i].value == CACHE_VAL_UNSET) continue;
            if (FN(cache)[i].depth == DEPTH_SOLVED) continue;
            
            uint32_t bi = FN(cache)[i].depth / binW;
            if (bi >= DEPTH_BINS) bi = DEPTH_BINS - 1;
            stats->depthBins[bi]++;
        }
    }
#endif

    // Calculate Board Vis Aggregates
    for (int k = 0; k < 14; k++) {
        if (k == 6 || k == 13) continue;

        uint64_t sum = 0;
        uint64_t count = 0;
        int max = 0;
        uint64_t riskCount = 0;
        
        for (int s = 0; s <= cap; s++) {
            if (hist[k][s] > 0) {
                uint64_t n = hist[k][s];
                sum += (s * n);
                count += n;
                if (s > max) max = s;
                if (s > stats->riskThreshold) riskCount += n;
            }
        }
        
        if (count > 0) {
            stats->avgStones[k] = (double)sum / (double)count;
            stats->maxStones[k] = (double)max;
            stats->riskStones[k] = (double)riskCount / (double)count * 100.0;
        } else {
            stats->avgStones[k] = 0;
            stats->maxStones[k] = 0;
            stats->riskStones[k] = 0;
        }
    }
}

#undef TAG_TYPE
