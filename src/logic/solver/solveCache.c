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
uint64_t invalidReads;
uint64_t hits;

uint32_t getCacheSize() {
    return cacheSize;
}

uint64_t translateBoard(Board* board, int8_t window) {
    uint64_t hash = 0;

    // Make first bit the current player
    hash |= ((uint64_t)(board->color == 1 ? 1 : 0));

    int offset = 1;
    uint8_t value;

    // First 2 get 4 bits
    for (int i = 0; i < 2; i++) {
        value = board->cells[i];
        if (value > 15) {
            return INVALID_HASH;
        }
        hash |= ((uint64_t)(value & 0x0F)) << offset;
        offset += 4;
    }

    // Next 4 get 5 bits
    for (int i = 2; i < 6; i++) {
        value = board->cells[i];
        if (value > 31) {
            return INVALID_HASH;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    // Other player first 2 get 4 bits
    for (int i = 7; i < 9; i++) {
        value = board->cells[i];
        if (value > 15) {
            return INVALID_HASH;
        }
        hash |= ((uint64_t)(value & 0x0F)) << offset;
        offset += 4;
    }

    // Other player next 4 get 5 bits
    for (int i = 9; i < 13; i++) {
        value = board->cells[i];
        if (value > 31) {
            return INVALID_HASH;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    // Last 8 bits are window id
    hash |= ((uint64_t)window) << offset;

    return hash;
}

// Hash function to get into the array index
uint32_t primaryHash(uint64_t hash) {
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

void cacheNode(Board* board, int evaluation, int8_t window) {
    uint64_t hashValue = translateBoard(board, window);

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
    const uint32_t index = primaryHash(hashValue);

    // Check if exists already
    // This completely validates the entry
    if (cache[index].key == hashValue) {
        return;
    }

    // Check if the entry is empty
    if (cache[index].value == UNSET_VALUE) {
        Entry entry = {entryValue, hashValue};
        cache[index] = entry;
    }

    // If we get here, that means the key is occupied by another entry, we always overwrite
    // We hope that newer keys are more likely to be accessed
    overwrites++;
    cache[index].value = entryValue;
    cache[index].key = hashValue;
}

int getCachedValue(Board* board, int8_t window) {
    uint64_t hashValue = translateBoard(board, window);

    // Not hashable
    if (hashValue == INVALID_HASH) {
        return INT32_MIN;
    }

    // Get the primary hash
    const int index = primaryHash(hashValue);

    // Check if the entry is unset
    if (cache[index].value == UNSET_VALUE) {
        return INT32_MIN;
    }

    // Check if the key matches
    if (cache[index].key != hashValue) {
        invalidReads++;
        return INT32_MIN;
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

int compareChunks(const void *a, const void *b) {
    return ((Chunk*)b)->size - ((Chunk*)a)->size;
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
    qsort(chunks, chunkCount, sizeof(Chunk), compareChunks);

    // Print the largest OUTPUT_CHUNK_COUNT chunks
    for (int i = 0; i < OUTPUT_CHUNK_COUNT && i < chunkCount; i++) {
        sprintf(message, "     %s   | %10d  | %10d", chunks[i].type ? "Set  " : "Unset", chunks[i].start, chunks[i].size);
        renderOutput(message, CHEAT_PREFIX);
    }

    renderOutput("  ---------------------------------------", CHEAT_PREFIX);

    free(chunks);

    sprintf(message, "  Overwrites: %lld", overwrites);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Invalid reads: %lld", invalidReads);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Hits: %lld", hits);
    renderOutput(message, CHEAT_PREFIX);

    hits = 0;
    invalidReads = 0;
    overwrites = 0;
}
