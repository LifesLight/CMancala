/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/solveCache.h"

uint64_t cacheSize = 0;
uint32_t cacheSizePow = 0;

// Perf Tracking
uint64_t hits;
uint64_t hitsLegalDepth;
uint64_t sameKeyOverwriteCount;
uint64_t victimOverwriteCount;
uint64_t lastHits;
uint64_t lastHitsLegalDepth;
uint64_t lastSameKeyOverwriteCount;
uint64_t lastVictimOverwriteCount;


typedef struct {
    uint64_t validation;

    int16_t value;
    uint16_t depth;

    int8_t boundType;

    // Replacement Policy Data
    uint8_t  gen;

    /**
     * 3 Spare Bytes
     */
} Entry;

Entry* cache;

static uint8_t currentGen = 1;

// Hash function to get into the array index
uint64_t indexHash(uint64_t hash) {
    // Fibonacci Hash
    return (uint32_t)(((hash * 11400714819323198485ULL) >> (64 + BUCKET_POW - cacheSizePow)) * BUCKET_ELEMENTS);
}

// Get Wrapparound save age
uint8_t getAge(uint8_t gen) {
    return (uint8_t)(currentGen - gen);
}

// Replacement Policy, returns true if we should discard a instead of b.
bool worseToKeep(const Entry *a, const Entry *b) {
    if (a->depth != b->depth) {
        return a->depth < b->depth;
    }

    const int aExact = (a->boundType == EXACT_BOUND);
    const int bExact = (b->boundType == EXACT_BOUND);
    if (aExact != bExact) {
        return aExact < bExact;
    }

    return getAge(a->gen) > getAge(b->gen);
}

void cacheNode(Board* board, int evaluation, int boundType, int depth, bool solved) {
    // Compute evaluation without score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    evaluation -= scoreDelta;

    // Translate the board to a unique hash value
    uint64_t hashValue;
    if (!translateBoard(board, &hashValue)) {
        return;
    }

    // Compute primary index and validation
    const uint64_t base = indexHash(hashValue);
    Entry *b = &cache[base];

    if (solved) {
        depth = UINT16_MAX;
    }

    // same-key update
    for (int i = 0; i < BUCKET_ELEMENTS; i++) {
        if (b[i].validation == hashValue) {
            if (b[i].depth > depth) return;

            b[i].validation = hashValue;
            b[i].value = evaluation;
            b[i].depth = depth;
            b[i].boundType = boundType;
            b[i].gen = currentGen;

            sameKeyOverwriteCount++;
            return;
        }
    }

    // empty slot
    for (int i = 0; i < BUCKET_ELEMENTS; i++) {
        if (b[i].validation == UNSET_VALIDATION) {
            b[i].validation = hashValue;
            b[i].value = evaluation;
            b[i].depth = depth;
            b[i].boundType = boundType;
            b[i].gen = currentGen;
            return;
        }
    }

    // pick victim
    int victim = 0;
    for (int i = 1; i < BUCKET_ELEMENTS; i++) {
        if (worseToKeep(&b[i], &b[victim])) {
            victim = i;
        }
    }

    victimOverwriteCount++;

    b[victim].validation = hashValue;
    b[victim].value = evaluation;
    b[victim].depth = depth;
    b[victim].boundType = boundType;
    b[victim].gen = currentGen;

    return;
}

bool getCachedValue(Board* board, int currentDepth, int *eval, int *boundType, bool *solved) {
    uint64_t hashValue;
    if (!translateBoard(board, &hashValue)) {
        return false;
    }

    // Get the primary hash
    const uint64_t indexCalc = indexHash(hashValue);

    uint64_t index = UINT64_MAX;
    for (int i = 0; i < BUCKET_ELEMENTS; i++) {
        if (cache[indexCalc + i].validation == hashValue) {
            index = indexCalc + i;
            break;
        }
    }

    // Check if the key matches
    if (index == UINT64_MAX) {
        return false;
    }

    // We have a hit
    hits++;
    Entry *e = &cache[index];

    // Our cached entry needs to be at least as deep as our current depth.
    if (e->depth < currentDepth) {
        return false;
    }

    hitsLegalDepth++;

    // Adjust for score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    *eval = e->value + scoreDelta;
    *boundType = e->boundType;
    *solved = e->depth == UINT16_MAX;

    return true;
}

uint64_t getCacheSize() {
    return cacheSize;
}

void startCache(int sizePow) {
    // sizePow == 0 means disabled
    if (sizePow == 0) {
        cacheSize = 0;
    } else {
        cacheSize = pow(2, sizePow);
        cacheSizePow = sizePow;
    }

    if (sizePow <= BUCKET_POW) {
        cacheSize = 0;
    }

    free(cache);
    cache = malloc(sizeof(Entry) * cacheSize);

    if (cache == NULL && cacheSize > 0) {
        renderOutput("Failed to allocate cache!", CONFIG_PREFIX);
        cacheSize = 0;
    }

    for (int i = 0; i < (int64_t)cacheSize; i++) {
        cache[i].value = 0;
        cache[i].validation = UNSET_VALIDATION;
        cache[i].gen = 0;
        cache[i].boundType = 0;
        cache[i].depth = 0;
    }

    hits = 0;
    hitsLegalDepth = 0;
    sameKeyOverwriteCount = 0;
    victimOverwriteCount = 0;

    lastHits = 0;
    lastHitsLegalDepth = 0;
    lastSameKeyOverwriteCount = 0;
    lastVictimOverwriteCount = 0;
}

/**
 * Returns false if cant be translated
*/
bool translateBoard(Board* board, uint64_t *code) {
    uint64_t h = 0;
    int offset = 0;

    const int a0 = (board->color == 1) ? 0 : 7;
    const int b0 = (board->color == 1) ? 7 : 0;

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

    *code = h;
    return true;
}

void stepCache() {
    currentGen++;
}

/**
 * Render the cache stats
*/
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

void resetCacheStats() {
    lastHits = hits;
    lastHitsLegalDepth = hitsLegalDepth;
    lastSameKeyOverwriteCount = sameKeyOverwriteCount;
    lastVictimOverwriteCount = victimOverwriteCount;

    hits = 0;
    hitsLegalDepth = 0;
    sameKeyOverwriteCount = 0;
    victimOverwriteCount = 0;
}

void renderCacheStats() {
    char message[256];
    char logBuffer[32];

    if (cacheSize == 0 || cache == NULL) {
        renderOutput("  Cache disabled.", CHEAT_PREFIX);
        return;
    }

    uint64_t setEntries = 0;
    uint64_t solvedEntries = 0;

    uint64_t exactCount = 0;
    uint64_t lowerCount = 0;
    uint64_t upperCount = 0;

    // Hits (Bad)
    const uint64_t badHits = lastHits - lastHitsLegalDepth;
    const double badHitPercent = (lastHits > 0) ? (double)badHits / (double)lastHits * 100.0 : 0.0;

    // Age + depth distributions
    uint64_t ageSum = 0;
    uint8_t  maxAge = 0;

    uint64_t nonSolvedCount = 0;
    uint64_t depthSum = 0;
    uint16_t maxDepth = 0;

    // Pass 1: gather totals and maxima
    for (uint64_t i = 0; i < cacheSize; i++) {
        if (cache[i].validation == UNSET_VALIDATION) continue;

        setEntries++;

        const bool solved = (cache[i].depth == UINT16_MAX);
        if (solved) {
            solvedEntries++;
        } else {
            nonSolvedCount++;
            depthSum += cache[i].depth;
            if (cache[i].depth > maxDepth) maxDepth = cache[i].depth;
        }

        if (cache[i].boundType == EXACT_BOUND) exactCount++;
        else if (cache[i].boundType == LOWER_BOUND) lowerCount++;
        else if (cache[i].boundType == UPPER_BOUND) upperCount++;

        const uint8_t age = getAge(cache[i].gen);
        ageSum += age;
        if (age > maxAge) maxAge = age;
    }

    // Cache Size / fill
    const double fillPct = (cacheSize > 0) ? (double)setEntries / (double)cacheSize * 100.0 : 0.0;
    getLogNotation(logBuffer, cacheSize);
    snprintf(message, sizeof(message),
             "  Cache size: %-12"PRIu64" %s (%.2f%% Used)", cacheSize, logBuffer, fillPct);
    renderOutput(message, CHEAT_PREFIX);

    // Solved
    const double solvedPct = (setEntries > 0) ? (double)solvedEntries / (double)setEntries * 100.0 : 0.0;
    getLogNotation(logBuffer, solvedEntries);
    snprintf(message, sizeof(message),
             "  Solved:     %-12"PRIu64" %s (%.2f%% of used)", solvedEntries, logBuffer, solvedPct);
    renderOutput(message, CHEAT_PREFIX);

    // Hits (Bad)
    getLogNotation(logBuffer, lastHits);
    snprintf(message, sizeof(message),
             "  Hits (Bad): %-12"PRIu64" %s (%.2f%%)", lastHits, logBuffer, badHitPercent);
    renderOutput(message, CHEAT_PREFIX);

    // Cache Overwrites header
    renderOutput("  Cache Overwrites:", CHEAT_PREFIX);

    // Same-key overwrites (indented under Cache Overwrites)
    getLogNotation(logBuffer, lastSameKeyOverwriteCount);
    snprintf(message, sizeof(message),
             "    Improve:  %-12"PRIu64" %s", lastSameKeyOverwriteCount, logBuffer);
    renderOutput(message, CHEAT_PREFIX);

    // Victim overwrites (indented under Cache Overwrites)
    getLogNotation(logBuffer, lastVictimOverwriteCount);
    snprintf(message, sizeof(message),
             "    Evict:    %-12"PRIu64" %s", lastVictimOverwriteCount, logBuffer);
    renderOutput(message, CHEAT_PREFIX);

    // Bounds distribution
    if (setEntries > 0) {
        snprintf(message, sizeof(message),
                 "  Bounds:     E %.2f%% | L %.2f%% | U %.2f%%",
                 (double)exactCount / (double)setEntries * 100.0,
                 (double)lowerCount / (double)setEntries * 100.0,
                 (double)upperCount / (double)setEntries * 100.0);
    } else {
        snprintf(message, sizeof(message),
                 "  Bounds:     E 0.00%% | L 0.00%% | U 0.00%%");
    }
    renderOutput(message, CHEAT_PREFIX);

    // Depth overview table
    if (nonSolvedCount > 0) {
        const double avgDepth = (double)depthSum / (double)nonSolvedCount;
        snprintf(message, sizeof(message),
                 "  Depth:      avg %.2f | max %u", avgDepth, (unsigned)maxDepth);
        renderOutput(message, CHEAT_PREFIX);

        enum { DEPTH_BINS = 8 };
        uint64_t depthBins[DEPTH_BINS] = {0};

        const uint32_t span = (uint32_t)maxDepth + 1;
        const uint32_t binW = (span + DEPTH_BINS - 1) / DEPTH_BINS;

        for (uint64_t i = 0; i < cacheSize; i++) {
            if (cache[i].validation == UNSET_VALIDATION) continue;
            if (cache[i].depth == UINT16_MAX) continue;
            const uint16_t d = cache[i].depth;
            uint32_t bi = (uint32_t)d / binW;
            if (bi >= DEPTH_BINS) bi = DEPTH_BINS - 1;
            depthBins[bi]++;
        }

        renderOutput("  Depth range| Count        | Percent", CHEAT_PREFIX);
        renderOutput("  ------------------------------------", CHEAT_PREFIX);

        for (uint32_t bi = 0; bi < DEPTH_BINS; bi++) {
            const uint32_t start = bi * binW;
            uint32_t end = start + binW - 1;
            if (end > (uint32_t)maxDepth) end = (uint32_t)maxDepth;

            const double pct = (double)depthBins[bi] / (double)nonSolvedCount * 100.0;
            snprintf(message, sizeof(message),
                     "  %3u-%-3u    | %-12"PRIu64"| %6.2f%%",
                     start, end, depthBins[bi], pct);
            renderOutput(message, CHEAT_PREFIX);

            if (end == (uint32_t)maxDepth) break;
        }

        renderOutput("  ------------------------------------", CHEAT_PREFIX);
    } else {
        renderOutput("  Depth:      avg 0.00 | max 0", CHEAT_PREFIX);
    }

    // Age overview table
    if (setEntries > 0) {
        enum { AGE_BINS = 8 };
        uint64_t ageBins[AGE_BINS] = {0};

        const uint32_t span = (uint32_t)maxAge + 1;
        const uint32_t binW = (span + AGE_BINS - 1) / AGE_BINS;

        for (uint64_t i = 0; i < cacheSize; i++) {
            if (cache[i].validation == UNSET_VALIDATION) continue;
            const uint8_t age = getAge(cache[i].gen);
            uint32_t bi = (uint32_t)age / binW;
            if (bi >= AGE_BINS) bi = AGE_BINS - 1;
            ageBins[bi]++;
        }

        const double avgAge = (double)ageSum / (double)setEntries;
        snprintf(message, sizeof(message),
                 "  Age(gen):   avg %.2f | max %u", avgAge, (unsigned)maxAge);
        renderOutput(message, CHEAT_PREFIX);

        renderOutput("  Age range  | Count        | Percent", CHEAT_PREFIX);
        renderOutput("  ------------------------------------", CHEAT_PREFIX);

        for (uint32_t bi = 0; bi < AGE_BINS; bi++) {
            const uint32_t start = bi * binW;
            uint32_t end = start + binW - 1;
            if (end > (uint32_t)maxAge) end = (uint32_t)maxAge;

            const double pct = (double)ageBins[bi] / (double)setEntries * 100.0;
            snprintf(message, sizeof(message),
                     "  %3u-%-3u    | %-12"PRIu64"| %6.2f%%",
                     start, end, ageBins[bi], pct);
            renderOutput(message, CHEAT_PREFIX);

            if (end == (uint32_t)maxAge) break;
        }

        renderOutput("  ------------------------------------", CHEAT_PREFIX);
    }

    renderOutput("  Fragmentation", CHEAT_PREFIX);
    renderOutput("  Chunk Type | Start Index       | Chunk Size", CHEAT_PREFIX);
    renderOutput("  --------------------------------------------", CHEAT_PREFIX);

    Chunk top[OUTPUT_CHUNK_COUNT];
    int topCount = 0;

    int currentType = (cache[0].validation != UNSET_VALIDATION);
    uint64_t chunkStart = 0;
    uint64_t chunkSize2 = 1;

    for (uint64_t i = 1; i < cacheSize; i++) {
        const int t = (cache[i].validation != UNSET_VALIDATION);
        if (t == currentType) {
            chunkSize2++;
        } else {
            Chunk c;
            c.type = currentType;
            c.start = chunkStart;
            c.size = chunkSize2;

            if (topCount < OUTPUT_CHUNK_COUNT) {
                top[topCount++] = c;
            } else {
                int minIdx = 0;
                for (int k = 1; k < topCount; k++) {
                    if (top[k].size < top[minIdx].size) minIdx = k;
                }
                if (c.size > top[minIdx].size) top[minIdx] = c;
            }

            currentType = t;
            chunkStart = i;
            chunkSize2 = 1;
        }
    }

    {
        Chunk c;
        c.type = currentType;
        c.start = chunkStart;
        c.size = chunkSize2;

        if (topCount < OUTPUT_CHUNK_COUNT) {
            top[topCount++] = c;
        } else {
            int minIdx = 0;
            for (int k = 1; k < topCount; k++) {
                if (top[k].size < top[minIdx].size) minIdx = k;
            }
            if (c.size > top[minIdx].size) top[minIdx] = c;
        }
    }

    qsort(top, topCount, sizeof(Chunk), compareChunksStart);

    for (int i = 0; i < topCount; i++) {
        snprintf(message, sizeof(message),
                 "     %s   | %17"PRIu64" | %17"PRIu64,
                 top[i].type ? "Set  " : "Unset",
                 top[i].start,
                 top[i].size);
        renderOutput(message, CHEAT_PREFIX);
    }

    if (topCount == OUTPUT_CHUNK_COUNT) {
        renderOutput("  ...", CHEAT_PREFIX);
    }

    renderOutput("  --------------------------------------------", CHEAT_PREFIX);
}
