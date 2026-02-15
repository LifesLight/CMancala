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
#endif

/**
 * Mixing here since the board rep is useless without correct mixing
 */

#if CACHE_B64
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
#else
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

static inline void FN(splitBoard)(uint64_t boardRep, uint64_t *index, TAG_TYPE *tag) {
#if CACHE_B64
    const int totalBits = 64;
#else
    const int totalBits = 48;
#endif

#if CACHE_T32
    const int tagBits = 32;
#else
    const int tagBits = 16;
#endif

    *index = boardRep & (cacheSize - 1);
    *tag   = (TAG_TYPE)(boardRep >> (totalBits - tagBits));
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

    FN(Entry) *b = &FN(cache)[index];

#if CACHE_DEPTH
    if (solved) {
        depth = DEPTH_SOLVED;
    }
#else
    (void)depth;
    (void)solved;
#endif

    // Same-key update
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

    FN(Entry) *entryBase = &FN(cache)[index];
    FN(Entry) *e = NULL;

    if (entryBase[0].tag == tag) {
        e = &entryBase[0];
    } else if (entryBase[1].tag == tag) {
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

// --- Stats ---

static void FN(renderCacheStatsFull)() {
    char message[256];
    char logBuffer[32];

    uint64_t setEntries = 0;
    uint64_t exactCount = 0;
    uint64_t lowerCount = 0;
    uint64_t upperCount = 0;

#if CACHE_DEPTH
    uint64_t solvedEntries = 0;
    uint64_t nonSolvedCount = 0;
    uint64_t depthSum = 0;
    uint16_t maxDepth = 0;
#endif

    for (uint64_t i = 0; i < cacheSize; i++) {
        if (FN(cache)[i].value == CACHE_VAL_UNSET) continue;
        setEntries++;

        int bt = UNPACK_BOUND(FN(cache)[i].value);

        if (bt == EXACT_BOUND) exactCount++;
        else if (bt == LOWER_BOUND) lowerCount++;
        else if (bt == UPPER_BOUND) upperCount++;

#if CACHE_DEPTH
        if (FN(cache)[i].depth == DEPTH_SOLVED) {
            solvedEntries++;
        } else {
            nonSolvedCount++;
            depthSum += FN(cache)[i].depth;
            if (FN(cache)[i].depth > maxDepth) maxDepth = FN(cache)[i].depth;
        }
#endif
    }

    char modeStr[128];
    const char* depthStr = 
#if CACHE_DEPTH
        "Depth";
#else
        "No Depth";
#endif

    const char* keyStr = 
#if CACHE_B64
        "64-bit Key";
#else
        "48-bit Key";
#endif

    const char* tagStr = 
#if CACHE_T32
        "32-bit Tag";
#else
        "16-bit Tag";
#endif

    snprintf(modeStr, sizeof(modeStr), "  Mode:       %s / %s / %s (%zu Bytes)", 
             depthStr, keyStr, tagStr, sizeof(FN(Entry)));
    renderOutput(modeStr, CHEAT_PREFIX);

    const double fillPct = (cacheSize > 0) ? (double)setEntries / (double)cacheSize * 100.0 : 0.0;
    getLogNotation(logBuffer, cacheSize);
    snprintf(message, sizeof(message), "  Cache size: %-12"PRIu64" %s (%.2f%% Used)", cacheSize, logBuffer, fillPct);
    renderOutput(message, CHEAT_PREFIX);

#if CACHE_DEPTH
    const double solvedPct = (setEntries > 0) ? (double)solvedEntries / (double)setEntries * 100.0 : 0.0;
    getLogNotation(logBuffer, solvedEntries);
    snprintf(message, sizeof(message), "  Solved:     %-12"PRIu64" %s (%.2f%% of used)", solvedEntries, logBuffer, solvedPct);
    renderOutput(message, CHEAT_PREFIX);
#endif

    getLogNotation(logBuffer, lastHits);
    snprintf(message, sizeof(message), "  Hits:       %-12"PRIu64" %s", lastHits, logBuffer);
    renderOutput(message, CHEAT_PREFIX);

#if CACHE_DEPTH
    const uint64_t badHits = lastHits - lastHitsLegalDepth;
    const double badHitPercent = (lastHits > 0) ? (double)badHits / (double)lastHits * 100.0 : 0.0;
    getLogNotation(logBuffer, badHits);
    snprintf(message, sizeof(message), "    Shallow:  %-12"PRIu64" %s (%.2f%%)", badHits, logBuffer, badHitPercent);
    renderOutput(message, CHEAT_PREFIX);
#endif

    const double swapPct = (lastHits > 0) ? (double)lastSwapLRUCount / (double)lastHits * 100.0 : 0.0;
    getLogNotation(logBuffer, lastSwapLRUCount);
    snprintf(message, sizeof(message), "    LRU Swap: %-12"PRIu64" %s (%.2f%%)", lastSwapLRUCount, logBuffer, swapPct);
    renderOutput(message, CHEAT_PREFIX);

    renderOutput("  Cache Overwrites:", CHEAT_PREFIX);
    getLogNotation(logBuffer, lastSameKeyOverwriteCount);
    snprintf(message, sizeof(message), "    Improve:  %-12"PRIu64" %s", lastSameKeyOverwriteCount, logBuffer);
    renderOutput(message, CHEAT_PREFIX);

    getLogNotation(logBuffer, lastVictimOverwriteCount);
    snprintf(message, sizeof(message), "    Evict:    %-12"PRIu64" %s", lastVictimOverwriteCount, logBuffer);
    renderOutput(message, CHEAT_PREFIX);

    if (lastFailedEncodeStoneCount > 0 || lastFailedEncodeValueRange > 0) {
        renderOutput("  Encoding Fail Counts:", CHEAT_PREFIX);
        getLogNotation(logBuffer, lastFailedEncodeStoneCount);
        snprintf(message, sizeof(message), "    Stones:   %-12"PRIu64" %s", lastFailedEncodeStoneCount, logBuffer);
        renderOutput(message, CHEAT_PREFIX);
        getLogNotation(logBuffer, lastFailedEncodeValueRange);
        snprintf(message, sizeof(message), "    Value:    %-12"PRIu64" %s", lastFailedEncodeValueRange, logBuffer);
        renderOutput(message, CHEAT_PREFIX);
    }

    if (setEntries > 0) {
        snprintf(message, sizeof(message), "  Bounds:     E %.2f%% | L %.2f%% | U %.2f%%",
                 (double)exactCount / (double)setEntries * 100.0,
                 (double)lowerCount / (double)setEntries * 100.0,
                 (double)upperCount / (double)setEntries * 100.0);
    } else {
        snprintf(message, sizeof(message), "  Bounds:     E 0.00%% | L 0.00%% | U 0.00%%");
    }
    renderOutput(message, CHEAT_PREFIX);

#if CACHE_DEPTH
    if (nonSolvedCount > 0) {
        const double avgDepth = (double)depthSum / (double)nonSolvedCount;
        snprintf(message, sizeof(message), "  Depth:      avg %.2f | max %u", avgDepth, (unsigned)maxDepth);
        renderOutput(message, CHEAT_PREFIX);

        enum { DEPTH_BINS = 8 };
        uint64_t depthBins[DEPTH_BINS] = {0};
        const uint32_t span = (uint32_t)maxDepth + 1;
        const uint32_t binW = (span + DEPTH_BINS - 1) / DEPTH_BINS;

        for (uint64_t i = 0; i < cacheSize; i++) {
            if (FN(cache)[i].value == CACHE_VAL_UNSET) continue;
            if (FN(cache)[i].depth == DEPTH_SOLVED) continue;
            uint32_t bi = FN(cache)[i].depth / binW;
            if (bi >= DEPTH_BINS) bi = DEPTH_BINS - 1;
            depthBins[bi]++;
        }

        renderOutput("  Depth range| Count        | Percent", CHEAT_PREFIX);
        renderOutput("  ------------------------------------", CHEAT_PREFIX);

        for (uint32_t bi = 0; bi < DEPTH_BINS; bi++) {
            const uint32_t start = bi * binW;
            uint32_t end = start + binW - 1;
            if (end > maxDepth) end = maxDepth;
            const double pct = (double)depthBins[bi] / (double)nonSolvedCount * 100.0;
            snprintf(message, sizeof(message), "  %3u-%-3u    | %-12"PRIu64"| %6.2f%%", start, end, depthBins[bi], pct);
            renderOutput(message, CHEAT_PREFIX);
            if (end == maxDepth) break;
        }
        renderOutput("  ------------------------------------", CHEAT_PREFIX);
    } else {
        renderOutput("  Depth:      avg 0.00 | max 0", CHEAT_PREFIX);
    }
#endif

    renderOutput("  Fragmentation", CHEAT_PREFIX);
    renderOutput("  Chunk Type | Start Index       | Chunk Size", CHEAT_PREFIX);
    renderOutput("  --------------------------------------------", CHEAT_PREFIX);

    Chunk top[OUTPUT_CHUNK_COUNT];
    int topCount = 0;

    int currentType = (FN(cache)[0].value != CACHE_VAL_UNSET);
    uint64_t chunkStart = 0;
    uint64_t chunkSize2 = 1;

    for (uint64_t i = 1; i < cacheSize; i++) {
        const int t = (FN(cache)[i].value != CACHE_VAL_UNSET);
        if (t == currentType) {
            chunkSize2++;
        } else {
            Chunk c = { chunkStart, chunkSize2, currentType };
            if (topCount < OUTPUT_CHUNK_COUNT) {
                top[topCount++] = c;
            } else {
                int minIdx = 0;
                for (int k = 1; k < topCount; k++) if (top[k].size < top[minIdx].size) minIdx = k;
                if (c.size > top[minIdx].size) top[minIdx] = c;
            }
            currentType = t;
            chunkStart = i;
            chunkSize2 = 1;
        }
    }

    Chunk c = { chunkStart, chunkSize2, currentType };
    if (topCount < OUTPUT_CHUNK_COUNT) {
        top[topCount++] = c;
    } else {
        int minIdx = 0;
        for (int k = 1; k < topCount; k++) if (top[k].size < top[minIdx].size) minIdx = k;
        if (c.size > top[minIdx].size) top[minIdx] = c;
    }

    qsort(top, topCount, sizeof(Chunk), compareChunksStart);

    for (int i = 0; i < topCount; i++) {
        snprintf(message, sizeof(message), "     %s   | %17"PRIu64" | %17"PRIu64,
                 top[i].type ? "Set  " : "Unset", top[i].start, top[i].size);
        renderOutput(message, CHEAT_PREFIX);
    }
    if (topCount == OUTPUT_CHUNK_COUNT) renderOutput("  ...", CHEAT_PREFIX);
    renderOutput("  --------------------------------------------", CHEAT_PREFIX);
}

#undef TAG_TYPE
