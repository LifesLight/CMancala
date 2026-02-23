#pragma once

/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "config.h"

typedef enum {
    UNIFORM_DIST,
    RANDOM_DIST
} Distribution;

typedef enum {
    HUMAN_AGENT,
    RANDOM_AGENT,
    AI_AGENT
} Agent;

typedef enum {
    CLASSIC_MOVE,
    AVALANCHE_MOVE
} MoveFunction;

typedef enum {
    LOCAL_SOLVER,
    GLOBAL_SOLVER
} Solver;

typedef enum {
    ALWAYS_COMPRESS,
    NEVER_COMPRESS,
    AUTO
} CacheMode;

typedef struct {
    Solver solver;
    int depth;
    double timeLimit;
    bool clip;
    CacheMode compressCache;
    bool progressBar;
} SolverConfig;

typedef struct {
    int stones;
    Distribution distribution;
    int seed;
    uint8_t startColor;
    Agent player1;
    Agent player2;
} GameSettings;

typedef struct {
    bool autoplay;
    GameSettings gameSettings;
    SolverConfig solverConfig;
} Config;

typedef struct {
    int lastEvaluation;
    int lastMove;
    int lastDepth;
    bool lastSolved;
    double lastTime;
    double lastDepthTimes[MAX_DEPTH];
    uint64_t lastNodes;
} Metadata;

typedef struct {
    uint8_t cells[ASIZE];
    int8_t color;
} Board;

typedef struct {
    int score;
    int8_t* moves;
} NegamaxTrace;

typedef struct {
    Board* board;
    Board* lastBoard;
    Config config;
    Metadata metadata;
} Context;

typedef struct {
    uint64_t start;
    uint64_t size;
    int type;
} CacheChunk;

typedef struct {
    // Configuration
    char modeStr[128];
    uint64_t cacheSize;
    size_t entrySize;
    bool hasDepth;

    // Usage Stats
    uint64_t setEntries;
    uint64_t exactCount;
    uint64_t lowerCount;
    uint64_t upperCount;

    // Depth Stats
    uint64_t solvedEntries;
    uint64_t nonSolvedCount;
    uint64_t depthSum;
    uint16_t maxDepth;
    uint64_t depthBins[8];

    // Performance Counters (Snapshots)
    uint64_t hits;
    uint64_t hitsLegal;
    uint64_t lruSwaps;
    uint64_t overwriteImprove;
    uint64_t overwriteEvict;
    uint64_t failStones;
    uint64_t failRange;

    // Board Visualization Data (Arrays of 14)
    double avgStones[14];
    double maxStones[14];
    double over7[14];
    double over15[14];
    int riskThreshold;

    // Fragmentation
    CacheChunk topChunks[OUTPUT_CHUNK_COUNT];
    int chunkCount;
} CacheStats;
