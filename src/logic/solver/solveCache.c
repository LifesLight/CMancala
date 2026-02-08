/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/solveCache.h"

typedef struct {
    uint64_t validation;

    int16_t value;
    uint16_t depth;

    int8_t boundType;

    // Replacement Policy Data
    uint8_t  gen;
    uint8_t  uses;
} Entry;

static uint8_t currentGen = 1;

Entry* cache;

int cacheSize = 0;
uint32_t cacheSizePow = 0;

uint64_t hits;
uint64_t hitsLegalDepth;
uint64_t lastHits;
uint64_t lastHitsLegalDepth;


uint32_t getCacheSize() {
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

    for (int i = 0; i < (int64_t)cacheSize; i++) {
        cache[i].value = 0;
        cache[i].validation = UNSET_VALIDATION;
        cache[i].gen = 0;
        cache[i].uses = 0;
        cache[i].boundType = 0;
        cache[i].depth = 0;
    }

    hits = 0;
    hitsLegalDepth = 0;

    lastHits = 0;
    lastHitsLegalDepth = 0;
}

/**
 * Returns false if cant be translated
*/
bool translateBoard(Board* board, uint64_t *code) {
    // Check if window can be stored
    uint64_t hash = 0;

    // Make first bit the current player
    hash |= ((uint64_t)((board->color == 1) ? 1 : 0));

    int offset = 1;
    uint8_t value;

    // 5 bits per cell
    for (int i = 0; i < 6; i++) {
        value = board->cells[i];
        if (value > 31) {
            return false;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    for (int i = 7; i < 13; i++) {
        value = board->cells[i];
        if (value > 31) {
            return false;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    *code = hash;
    return true;
}

// Hash function to get into the array index
uint32_t indexHash(uint64_t hash) {
    // Fibonacci Hash
    return (uint32_t)(((hash * 11400714819323198485ULL) >> (64 + BUCKET_POW - cacheSizePow)) * BUCKET_ELEMENTS);
}

static inline uint8_t ageDiff(uint8_t now, uint8_t then) {
    return (uint8_t)(now - then);
}

static inline int keepScore(const Entry *e) {
    int s = 0;
    if (e->depth == UINT16_MAX) {
        s += 4000;
    } else {
        s += e->depth;
    }

    if (e->boundType == EXACT_BOUND)    s += 2000;

    s += (int)                          (e->uses >> 4);
    s -= (int)                          ageDiff(currentGen, e->gen) * 8;

    return s;
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
    const uint32_t base = indexHash(hashValue);
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
            if (b[i].uses < 255) b[i].uses++; else b[i].uses = 255;
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
            b[i].uses = 1;
            return;
        }
    }

    // pick victim = lowest keepScore
    int victim = 0;
    int worst = keepScore(&b[0]);
    for (int i = 1; i < BUCKET_ELEMENTS; i++) {
        int sc = keepScore(&b[i]);
        if (sc < worst) { 
            worst = sc; victim = i;
        }
    }

    b[victim].validation = hashValue;
    b[victim].value = evaluation;
    b[victim].depth = depth;
    b[victim].boundType = boundType;
    b[victim].gen = currentGen;
    b[victim].uses = 1;

    return;
}

bool getCachedValue(Board* board, int currentDepth, int *eval, int *boundType, bool *solved) {
    uint64_t hashValue;
    if (!translateBoard(board, &hashValue)) {
        return false;
    }

    // Get the primary hash
    const int indexCalc = indexHash(hashValue);

    int index = -1;
    for (int i = 0; i < BUCKET_ELEMENTS; i++) {
        if (cache[indexCalc + i].validation == hashValue) {
            index = indexCalc + i;
            break;
        }
    }

    // Check if the key matches
    if (index == -1) {
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
    if (e->uses < UINT8_MAX) {
        e->uses++;
    }

    // Adjust for score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    *eval = e->value + scoreDelta;
    *boundType = e->boundType;
    *solved = e->depth == UINT16_MAX;
    return true;
}

/**
 * Render the cache stats
*/
typedef struct {
    int start;
    int size;
    int type;
} Chunk;

int compareChunksSize(const void *a, const void *b) {
    return ((Chunk*)b)->size - ((Chunk*)a)->size;
}

int compareChunksStart(const void *a, const void *b) {
    return ((Chunk*)a)->start - ((Chunk*)b)->start;
}

void renderCacheStats() {
    char message[256];
    char logBuffer[32]; 

    uint64_t setEntries = 0;
    uint64_t solvedEntries = 0;

    for (int i = 0; i < (int64_t)cacheSize; i++) {
        if (cache[i].validation != UNSET_VALIDATION) {
            setEntries++;

            if (cache[i].depth == UINT16_MAX) {
                solvedEntries++;
            }
        }
    }

    double fillPercentage = (double)setEntries / (double)cacheSize;

    // Cache Size
    getLogNotation(logBuffer, cacheSize);
    sprintf(message, "  Cache size: %-12d %s (%.2f%% Used)", cacheSize, logBuffer, fillPercentage * 100);
    renderOutput(message, CHEAT_PREFIX);

    double solvedPercentage = 0.0;
    if (setEntries > 0) {
        solvedPercentage = (double)solvedEntries / (double)setEntries * 100.0;
    }
    getLogNotation(logBuffer, solvedEntries);
    sprintf(message, "  Solved:     %-12"PRIu64" %s (%.2f%%)", solvedEntries, logBuffer, solvedPercentage);
    renderOutput(message, CHEAT_PREFIX);

    // Hits (Bad)
    uint64_t badHits = lastHits - lastHitsLegalDepth;
    double badHitPercent = 0.0;
    if (lastHits > 0) {
        badHitPercent = (double)badHits / (double)lastHits * 100.0;
    }

    getLogNotation(logBuffer, lastHits);
    sprintf(message, "  Hits (Bad): %-12"PRIu64" %s (%.2f%%)", lastHits, logBuffer, badHitPercent);
    renderOutput(message, CHEAT_PREFIX);

    renderOutput("  Fragmentation", CHEAT_PREFIX);
    renderOutput("  Chunk Type | Start Index | Chunk Size", CHEAT_PREFIX);
    renderOutput("  ---------------------------------------", CHEAT_PREFIX);

    Chunk* chunks = malloc(sizeof(Chunk) * cacheSize);
    int chunkCount = 0;
    int currentType = cache[0].validation != UNSET_VALIDATION;
    int chunkStart = 0;
    int chunkSize = 1;

    for (int i = 1; i < (int64_t)cacheSize; i++) {
        if ((cache[i].validation != UNSET_VALIDATION) == currentType) { 
            chunkSize++;
        } else {
            chunks[chunkCount].type = currentType;
            chunks[chunkCount].start = chunkStart;
            chunks[chunkCount].size = chunkSize;
            chunkCount++;

            currentType = cache[i].validation != UNSET_VALIDATION;
            chunkStart = i;
            chunkSize = 1;
        }
    }

    chunks[chunkCount].type = currentType;
    chunks[chunkCount].start = chunkStart;
    chunks[chunkCount].size = chunkSize;
    chunkCount++;

    qsort(chunks, chunkCount, sizeof(Chunk), compareChunksSize);
    Chunk* topChunks = malloc(sizeof(Chunk) * min(chunkCount, OUTPUT_CHUNK_COUNT));

    for (int i = 0; i < min(chunkCount, OUTPUT_CHUNK_COUNT); i++) {
        topChunks[i] = chunks[i];
    }

    qsort(topChunks, min(chunkCount, OUTPUT_CHUNK_COUNT), sizeof(Chunk), compareChunksStart);

    for (int i = 0; i < min(chunkCount, OUTPUT_CHUNK_COUNT); i++) {
        sprintf(message, "     %s   | %10d  | %10d", topChunks[i].type ? "Set  " : "Unset", topChunks[i].start, topChunks[i].size);
        renderOutput(message, CHEAT_PREFIX);
    }

    if (chunkCount > OUTPUT_CHUNK_COUNT) {
        renderOutput("  ...", CHEAT_PREFIX);
    }

    renderOutput("  ---------------------------------------", CHEAT_PREFIX);

    free(chunks);
    free(topChunks);
}

void stepCache() {
    currentGen++;
}

void resetCacheStats() {
    lastHits = hits;
    lastHitsLegalDepth = hitsLegalDepth;

    hits = 0;
    hitsLegalDepth = 0;
}
