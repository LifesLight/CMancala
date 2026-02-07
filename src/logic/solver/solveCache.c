/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/solveCache.h"

typedef struct {
    uint64_t validation;

    int8_t value;
    int8_t boundType;
    uint8_t depth;
} Entry;

Entry* cache;

int cacheSize = 0;
uint32_t cacheSizePow = 0;

uint64_t overwrites;
uint64_t lastOverwrites;
uint64_t invalidReads;
uint64_t lastInvalidReads;
uint64_t hits;
uint64_t hitsLegalDepth;
uint64_t lastHits;
uint64_t lastHitsLegalDepth;


uint32_t getCacheSize() {
    return cacheSize;
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
    return (uint32_t)((hash * 11400714819323198485ULL) >> (64 - cacheSizePow));
}

void startCache(int sizePow) {
    cacheSize = pow(2, sizePow);
    cacheSizePow = sizePow;
    free(cache);
    cache = malloc(sizeof(Entry) * cacheSize);

    for (int i = 0; i < (int64_t)cacheSize; i++) {
        cache[i].value = UNSET_VALUE;
        cache[i].validation = 0;
        cache[i].boundType = 0;
        cache[i].depth = 0;
    }

    overwrites = 0;
    invalidReads = 0;
    hits = 0;
    hitsLegalDepth = 0;

    lastOverwrites = 0;
    lastInvalidReads = 0;
    lastHits = 0;
    lastHitsLegalDepth = 0;
}

void cacheNode(Board* board, int evaluation, int boundType, int depth, bool solved) {
    // Compute evaluation without score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    evaluation -= scoreDelta;

    if (evaluation > INT8_MAX || evaluation < INT8_MIN + 1) {
        return;
    }

    // Translate the board to a unique hash value
    uint64_t hashValue;
    if (!translateBoard(board, &hashValue)) {
        return;
    }

    // Compute primary index and validation
    const uint32_t index = indexHash(hashValue);

    // Check if the entry already exists
    if (cache[index].validation == hashValue) {
        // No reason to store a lower depth cache
        if (cache[index].depth > depth) {
            return;
        }

        // We will overwrite the current entry
        overwrites++;
    }

    // Update cache entry
    cache[index].boundType = (int8_t)((boundType & 0x03) | (solved ? 4 : 0));
    cache[index].validation = hashValue;
    cache[index].value = evaluation;
    cache[index].depth = depth;

    return;
}

bool getCachedValue(Board* board, int currentDepth, int *eval, int *boundType, bool *solved) {
    uint64_t hashValue;
    if (!translateBoard(board, &hashValue)) {
        return false;
    }

    // Get the primary hash
    const int index = indexHash(hashValue);

    // Check if the entry is unset
    if (cache[index].value == UNSET_VALUE) {
        return false;
    }

    // Check if the key matches
    if (cache[index].validation != hashValue) {
        invalidReads++;
        return false;
    }

    // We have a hit
    hits++;

    // Our cached entry needs to be at least as deep as our current depth.
    if (cache[index].depth < currentDepth) {
        return false;
    }

    hitsLegalDepth++;

    // Adjust for score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    *eval = cache[index].value + scoreDelta;
    *boundType = cache[index].boundType & 0x03;
    *solved = (cache[index].boundType & 4) != 0;
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
    char message[100];

    uint64_t setEntries = 0;

    for (int i = 0; i < (int64_t)cacheSize; i++) {
        if (cache[i].value != UNSET_VALUE) {
            setEntries++;
        }
    }

    double fillPercentage = (double)setEntries / (double)cacheSize;
    sprintf(message, "  Cache size: %d (%.2f", cacheSize, fillPercentage * 100);
    strcat(message, "%)");
    renderOutput(message, CHEAT_PREFIX);

    sprintf(message, "  Overwrites: %"PRIu64"", lastOverwrites);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Collisions: %"PRIu64"", lastInvalidReads);
    renderOutput(message, CHEAT_PREFIX);

    sprintf(message, "  Hits (Bad): %"PRIu64" (%"PRIu64")", lastHitsLegalDepth, lastHits - lastHitsLegalDepth);
    renderOutput(message, CHEAT_PREFIX);

    renderOutput("  Fragmentation", CHEAT_PREFIX);
    renderOutput("  Chunk Type | Start Index | Chunk Size", CHEAT_PREFIX);
    renderOutput("  ---------------------------------------", CHEAT_PREFIX);

    // Store all chunks
    Chunk* chunks = malloc(sizeof(Chunk) * cacheSize);
    int chunkCount = 0;
    int currentType = cache[0].value != UNSET_VALUE;
    int chunkStart = 0;
    int chunkSize = 1;

    // Iterate through the cache to identify chunks
    for (int i = 1; i < (int64_t)cacheSize; i++) {
        if ((cache[i].value != UNSET_VALUE) == currentType) { 
            chunkSize++;
        } else {
            chunks[chunkCount].type = currentType;
            chunks[chunkCount].start = chunkStart;
            chunks[chunkCount].size = chunkSize;
            chunkCount++;

            currentType = cache[i].value != UNSET_VALUE;
            chunkStart = i;
            chunkSize = 1;
        }
    }

    // Add the last chunk
    chunks[chunkCount].type = currentType;
    chunks[chunkCount].start = chunkStart;
    chunks[chunkCount].size = chunkSize;
    chunkCount++;

    // Sort the chunks by size in descending order
    qsort(chunks, chunkCount, sizeof(Chunk), compareChunksSize);

    // Allocate memory for top chunks
    Chunk* topChunks = malloc(sizeof(Chunk) * min(chunkCount, OUTPUT_CHUNK_COUNT));

    // Store top chunks separately
    for (int i = 0; i < min(chunkCount, OUTPUT_CHUNK_COUNT); i++) {
        topChunks[i] = chunks[i];
    }

    // Sort top chunks by start index in ascending order
    qsort(topChunks, min(chunkCount, OUTPUT_CHUNK_COUNT), sizeof(Chunk), compareChunksStart);

    // Print the largest OUTPUT_CHUNK_COUNT chunks
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
    lastOverwrites = overwrites;
    lastInvalidReads = invalidReads;
    lastHits = hits;
    lastHitsLegalDepth = hitsLegalDepth;

    overwrites = 0;
    invalidReads = 0;
    hits = 0;
    hitsLegalDepth = 0;
}
