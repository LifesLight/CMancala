/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "logic/solver/solveCache.h"

/**
 * We are using 8 validation bits for the hash
*/

typedef struct {
    int8_t value;
    uint64_t key;
} Entry;

Entry* cache;

uint32_t cacheSize = 0;
uint64_t overwrites;
uint64_t upgrades;
uint64_t invalidReads;
uint64_t hits;

uint32_t getCacheSize() {
    return cacheSize;
}

uint64_t translateBoard(Board* board, int alpha, int beta) {
    // Check if window can be stored
    if (alpha > 63 || alpha < -64 || beta > 63 || beta < -64) {
        return INVALID_HASH;
    }

    uint64_t hash = 0;

    // Make first bit the current player
    hash |= ((uint64_t)((board->color == 1) ? 1 : 0));

    int offset = 1;
    uint8_t value;

    // 4 bits per cell
    for (int i = 0; i < 6; i++) {
        value = board->cells[i];
        if (value > 15) {
            return INVALID_HASH;
        }
        hash |= ((uint64_t)(value & 0x0F)) << offset;
        offset += 4;
    }

    for (int i = 7; i < 13; i++) {
        value = board->cells[i];
        if (value > 15) {
            return INVALID_HASH;
        }
        hash |= ((uint64_t)(value & 0x0F)) << offset;
        offset += 4;
    }

    // Next 7 bits are the alpha window
    hash |= ((uint64_t)((alpha + 64) & 0x7F)) << offset;
    offset += 7;

    // Next 7 bits are the beta window
    hash |= ((uint64_t)((beta + 64) & 0x7F)) << offset;

    // Last bit can only be 1 if the hash is invalid
    return hash;
}

// Hash function to get into the array index
uint32_t indexHash(uint64_t hash) {
    return hash % cacheSize;
}

void startCache(uint32_t size) {
    cacheSize = size;
    free(cache);
    cache = malloc(sizeof(Entry) * cacheSize);
    for (int i = 0; i < (int64_t)cacheSize; i++) {
        cache[i].value = UNSET_VALUE;
        cache[i].key = 0;
    }

    overwrites = 0;
    invalidReads = 0;
    hits = 0;
}

void cacheNode(Board* board, int evaluation, int alpha, int beta) {
    uint64_t hashValue = translateBoard(board, alpha, beta);

    if (hashValue == INVALID_HASH) {
        return;
    }

    // Compute evaluation without score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    evaluation -= scoreDelta;

    if (evaluation > INT8_MAX || evaluation < INT8_MIN + 1) {
        return;
    }

    const int8_t entryValue = evaluation;

    // Get the primary hash
    const uint32_t index = indexHash(hashValue);

    // Check if exists already
    // This completely validates the entry
    if (cache[index].key == hashValue) {
        if (cache[index].value < entryValue) {
            upgrades++;
            cache[index].value = entryValue;
        }
        return;
    }

    // Check if the entry is empty
    if (cache[index].value != UNSET_VALUE) {
        overwrites++;
    }

    cache[index].value = entryValue;
    cache[index].key = hashValue;
}

int getCachedValue(Board* board, int alpha, int beta) {
    uint64_t hashValue = translateBoard(board, alpha, beta);

    // Not hashable
    if (hashValue == INVALID_HASH) {
        return NOT_CACHED_VALUE;
    }

    // Get the primary hash
    const int index = indexHash(hashValue);

    // Check if the entry is unset
    if (cache[index].value == UNSET_VALUE) {
        return NOT_CACHED_VALUE;
    }

    // Check if the key matches
    if (cache[index].key != hashValue) {
        invalidReads++;
        return NOT_CACHED_VALUE;
    }

    // We have a hit
    hits++;
    int value = cache[index].value;

    // Adjust for score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    return value + scoreDelta;
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
    sprintf(message, "  Cache size: %d", cacheSize);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Top %d cache chunks:", OUTPUT_CHUNK_COUNT);
    renderOutput(message, CHEAT_PREFIX);
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

    renderOutput("  ---------------------------------------", CHEAT_PREFIX);

    free(chunks);
    free(topChunks);

    sprintf(message, "  Overwrites: %lld", overwrites);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Upgrades: %lld", upgrades);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Caught: %lld", invalidReads);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Hits: %lld", hits);
    renderOutput(message, CHEAT_PREFIX);

    hits = 0;
    invalidReads = 0;
    upgrades = 0;
    overwrites = 0;
}
