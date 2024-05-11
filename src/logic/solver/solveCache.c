/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "logic/solver/solveCache.h"

int8_t cache[CACHE_SIZE];

uint64_t hashBoard(Board* board) {
    uint64_t hash = 0;

    // Make first bit the current player
    hash |= ((uint64_t)(board->color == 1 ? 1 : 0));

    int offset = 1;
    uint8_t value;

    for (int8_t i = HBOUND_P1; i >= LBOUND_P1; i--) {
        value = board->cells[i];
        if (value > 31) {
            return UINT64_MAX;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    for (int8_t i = HBOUND_P2; i >= LBOUND_P2; i--) {
        value = board->cells[i];
        if (value > 31) {
            return UINT64_MAX;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    return hash;
}

void startCache() {
    memset(cache, INT8_MIN, CACHE_SIZE);
}

void cacheNode(Board* board, int evaluation) {
    uint64_t hashValue = hashBoard(board);

    if (hashValue == UINT64_MAX) {
        return;
    }

    // Compute evaluation without score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    evaluation -= scoreDelta;

    cache[hashValue % CACHE_SIZE] = evaluation;
}

int getCachedValue(Board* board) {
    uint64_t hashValue = hashBoard(board);
    if (hashValue == UINT64_MAX) {
        return INT32_MIN;
    }

    // Check if inside cache
    int value = cache[hashValue % CACHE_SIZE];
    if (value == INT8_MIN) {
        return INT32_MIN;
    }

    // Adjust for score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    return value + scoreDelta;
}

int getCachedNodeCount() {
    int count = 0;

    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i] != INT8_MIN) {
            count++;
        }
    }

    return count;
}
