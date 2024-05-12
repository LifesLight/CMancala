/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "logic/solver/solveCache.h"

/**
 * We are using 8 validation bits for the hash
*/


int32_t cache[CACHE_SIZE];
uint64_t overwrites;
uint64_t invalidReads;
uint64_t hits;

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
    return hash % CACHE_SIZE;
}

// Secondary hash to compute the 8 check bits
uint32_t secondaryHash(uint64_t hash) {
    return hash % 16777216;
}

void startCache() {
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache[i] = UNSET_VALUE;
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
    const uint32_t entryCheck = secondaryHash(hashValue);

    // Merge both into the 32 bit entry
    const int32_t entry = (entryValue & 0xFF) | ((entryCheck & 0xFFFFFF) << 8);

    // Get the primary hash
    const uint32_t index = primaryHash(hashValue);

    // Check if the entry is already in the cache
    if (cache[index] == entry) {
        return;
    }

    // Check if the entry is empty
    if (cache[index] == UNSET_VALUE) {
        cache[index] = entry;
        return;
    }

    // Collision resolution
    overwrites++;

    // We always overwrite the entry
    cache[index] = entry;
}

int getCachedValue(Board* board, int8_t window) {
    uint64_t hashValue = translateBoard(board, window);

    // Not hashable
    if (hashValue == INVALID_HASH) {
        return INT32_MIN;
    }

    // Get the primary hash
    const int index = primaryHash(hashValue);

    // Check if the entry is empty
    if (cache[index] == UNSET_VALUE) {
        return INT32_MIN;
    }

    int32_t entry = cache[index];

    // Validate check bits
    const uint32_t entryCheck = secondaryHash(hashValue);
    if ((entryCheck & 0xFFFFFF) != ((entry >> 8) & 0xFFFFFF)) {
        invalidReads++;
        return INT32_MIN;
    }

    hits++;

    // Extract the evaluation
    int8_t value = entry & 0xFF;

    // Adjust for score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    return value + scoreDelta;
}

int getCachedNodeCount() {
    int count = 0;

    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i] != UNSET_VALUE) {
            count++;
        }
    }

    return count;
}

/**
 * Render the cache stats
*/
void renderCacheDistribution() {
    int chunkSize = 0;
    int chunkCount = 0;
    int chunkStart = 0;
    int chunkType = cache[0] != UNSET_VALUE; // 1 for set, 0 for unset


    char message[100];
    sprintf(message, "  Cache size: %d", CACHE_SIZE);
    sprintf(message, "  Top %d cache chunks:", OUTPUT_CHUNK_COUNT);
    renderOutput(message, CHEAT_PREFIX);
    renderOutput("  Chunk Type | Start Index | Chunk Size", CHEAT_PREFIX);
    renderOutput("  ---------------------------------------", CHEAT_PREFIX);

    // Array to store the largest 10 chunks
    int largestChunkSizes[OUTPUT_CHUNK_COUNT] = {0};
    int largestChunkStart[OUTPUT_CHUNK_COUNT] = {0};

    for (int i = 1; i < CACHE_SIZE; i++) {
        int currentType = cache[i] != UNSET_VALUE;
        if (currentType == chunkType) {
            chunkSize++;
        } else {
            if (chunkSize > largestChunkSizes[9]) {
                // Update the largest chunks array
                for (int j = 9; j >= 0; j--) {
                    if (chunkSize > largestChunkSizes[j]) {
                        if (j < 9) {
                            largestChunkSizes[j + 1] = largestChunkSizes[j];
                            largestChunkStart[j + 1] = largestChunkStart[j];
                        }
                        largestChunkSizes[j] = chunkSize;
                        largestChunkStart[j] = chunkStart;
                    } else {
                        break;
                    }
                }
            }
            chunkCount++;
            chunkType = currentType;
            chunkStart = i;
            chunkSize = 1;
        }
    }

    // Print the largest 10 chunks
    for (int i = 0; i < OUTPUT_CHUNK_COUNT && i < chunkCount; i++) {
        sprintf(message, "  %s | %10d | %10d", largestChunkSizes[i] ? "Set  " : "Unset", largestChunkStart[i], largestChunkSizes[i]);
        renderOutput(message, CHEAT_PREFIX);
    }


    sprintf(message, "  Overwrites: %d", chunkCount);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Invalid reads: %d", chunkCount);
    renderOutput(message, CHEAT_PREFIX);
    sprintf(message, "  Hits: %d", chunkCount);
    renderOutput(message, CHEAT_PREFIX);

    hits = 0;
    invalidReads = 0;
    overwrites = 0;
}
