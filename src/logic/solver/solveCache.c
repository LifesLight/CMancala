/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "logic/solver/solveCache.h"

int8_t cache[CACHE_SIZE];

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
            return UINT64_MAX;
        }
        hash |= ((uint64_t)(value & 0x0F)) << offset;
        offset += 4;
    }

    // Next 4 get 5 bits
    for (int i = 2; i < 6; i++) {
        value = board->cells[i];
        if (value > 31) {
            return UINT64_MAX;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    // Other player first 2 get 4 bits
    for (int i = 7; i < 9; i++) {
        value = board->cells[i];
        if (value > 15) {
            return UINT64_MAX;
        }
        hash |= ((uint64_t)(value & 0x0F)) << offset;
        offset += 4;
    }

    // Other player next 4 get 5 bits
    for (int i = 9; i < 13; i++) {
        value = board->cells[i];
        if (value > 31) {
            return UINT64_MAX;
        }
        hash |= ((uint64_t)(value & 0x1F)) << offset;
        offset += 5;
    }

    // Last 8 bits are window id
    hash |= ((uint64_t)window) << offset;

    return hash;
}

void startCache() {
    memset(cache, INT8_MIN, CACHE_SIZE);
}

void cacheNode(Board* board, int evaluation, int8_t window) {
    uint64_t hashValue = translateBoard(board, window);

    if (hashValue == UINT64_MAX) {
        return;
    }

    // Compute evaluation without score cells
    int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
    scoreDelta *= board->color;
    evaluation -= scoreDelta;

    if (evaluation > INT8_MAX || evaluation < INT8_MIN + 1) {
        return;
    }

    cache[hashValue % CACHE_SIZE] = evaluation;
}

int getCachedValue(Board* board, int8_t window) {
    uint64_t hashValue = translateBoard(board, window);
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
