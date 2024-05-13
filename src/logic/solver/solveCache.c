/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "logic/solver/solveCache.h"

typedef struct {
    int8_t value;
    uint64_t key;
#ifdef DYNAMIC_PERCENTAGE
    uint16_t accessCount;
#endif
} Entry;

Entry* cache;

#ifdef DYNAMIC_PERCENTAGE
uint64_t storedAccesses;
uint32_t setCells;
#endif

uint32_t cacheSize = 0;
uint64_t overwrites;
uint64_t invalidReads;
uint64_t hits;

uint32_t getCacheSize() {
    return cacheSize;
}

uint64_t translateBoard(Board* board) {
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
            return INVALID_HASH;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    for (int i = 7; i < 13; i++) {
        value = board->cells[i];
        if (value > 31) {
            return INVALID_HASH;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    // at least 1 bit needs to be free to check if the hash is invalid
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
#ifdef DYNAMIC_PERCENTAGE
        cache[i].accessCount = 0;
#endif
    }

    overwrites = 0;
    invalidReads = 0;
    hits = 0;
#ifdef DYNAMIC_PERCENTAGE
    storedAccesses = 0;
    setCells = 0;
#endif
}

void cacheNode(Board* board, int evaluation) {
    uint64_t hashValue = translateBoard(board);

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
            cache[index].value = entryValue;
        }
        return;
    }

#ifndef DYNAMIC_PERCENTAGE
    // Check if we need to overwrite
    if (cache[index].value != UNSET_VALUE) {
        overwrites++;
    }

    cache[index].key = hashValue;
    cache[index].value = entryValue;
    return;
#else
    // Check if the entry is unset
    if (cache[index].value == UNSET_VALUE) {
        cache[index].key = hashValue;
        cache[index].value = entryValue;
        cache[index].accessCount = 0;
        setCells++;
        return;
    }

    // Decide if we should overwrite
    // If it falls in the "irrelevant" category, overwrite
    if (cache[index].accessCount <= (DYNAMIC_PERCENTAGE * storedAccesses) / setCells) {
        cache[index].key = hashValue;
        cache[index].value = entryValue;
        storedAccesses -= cache[index].accessCount;
        cache[index].accessCount = 0;
        overwrites++;
        return;
    }
#endif
}

int getCachedValue(Board* board) {
    uint64_t hashValue = translateBoard(board);

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

#ifdef DYNAMIC_PERCENTAGE
    if (cache[index].accessCount < UINT16_MAX) {
        cache[index].accessCount++;
        storedAccesses++;
    }
#endif

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

    sprintf(message, "  Overwrites: %lld", overwrites);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Collisions: %lld", invalidReads);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Hits:       %lld", hits);
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

    if (chunkCount > OUTPUT_CHUNK_COUNT) {
        renderOutput("  ...", CHEAT_PREFIX);
    }

    renderOutput("  ---------------------------------------", CHEAT_PREFIX);


    free(chunks);
    free(topChunks);

#ifdef DYNAMIC_PERCENTAGE
    // Access distribution
    uint16_t smallestAccess = UINT16_MAX;
    uint16_t largestAccess = 0;
    uint64_t totalAccesses = 0;
    double mean = (double)storedAccesses / (double)setEntries;
    double std = 0;

    for (int i = 0; i < (int64_t)cacheSize; i++) {
        if (cache[i].value == UNSET_VALUE) {
            continue;
        }

        if (cache[i].accessCount < smallestAccess) {
            smallestAccess = cache[i].accessCount;
        }

        if (cache[i].accessCount > largestAccess) {
            largestAccess = cache[i].accessCount;
        }

        totalAccesses += cache[i].accessCount;
        std += pow(cache[i].accessCount - mean, 2);
    }

    if (totalAccesses != storedAccesses) {
        renderOutput("  Warning: Access count mismatch detected, fixing...", CHEAT_PREFIX);
        storedAccesses = totalAccesses;
    }

    sprintf(message, "  Access distribution: %d - %d", smallestAccess, largestAccess);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Mean: %.2f", mean);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Std: %.2f", sqrt(std / (double)cacheSize));
    renderOutput(message, CHEAT_PREFIX);
#endif
}
