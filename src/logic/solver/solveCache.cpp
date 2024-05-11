/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "logic/solver/solveCache.hpp"

#include <unordered_map>
using std::unordered_map;

unordered_map<uint64_t, int8_t> cache;

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

extern "C" {
    void startCache() {
        cache.clear();
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
        cache.insert({hashValue, evaluation});
    }

    int getCachedValue(Board* board) {
        uint64_t hashValue = hashBoard(board);

        if (hashValue == UINT64_MAX) {
            return INT32_MIN;
        }

        // Check if inside cache
        if (cache.find(hashValue) == cache.end()) {
            return INT32_MIN;
        }

        int8_t value = cache.at(hashValue);

        // Adjust for score cells
        int scoreDelta = board->cells[SCORE_P1] - board->cells[SCORE_P2];
        scoreDelta *= board->color;

        return value + scoreDelta;
    }

    int getCachedNodeCount() {
        return cache.size();
    }
}
