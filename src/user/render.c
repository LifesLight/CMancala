/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "user/render.h"

void renderCustomBoard(const int32_t *cells, const int8_t color, const char* prefix, const GameSettings* settings) {
    char* playerDescriptor = "Unknown";
    if (color == 1) {
        if (settings == NULL) {
            playerDescriptor = "Player 1";
        } else {
            switch (settings->player1) {
                case HUMAN_AGENT:
                    playerDescriptor = "Player 1";
                    break;
                case AI_AGENT:
                    playerDescriptor = "AI 1";
                    break;
                case RANDOM_AGENT:
                    playerDescriptor = "Random 1";
                    break;
            }
        }
    } else {
        if (settings == NULL) {
            playerDescriptor = "Player 2";
        } else {
            switch (settings->player2) {
                case HUMAN_AGENT:
                    playerDescriptor = "Player 2";
                    break;
                case AI_AGENT:
                    playerDescriptor = "AI 2";
                    break;
                case RANDOM_AGENT:
                    playerDescriptor = "Random 2";
                    break;
            }
        }
    }

    // Print indices
    printf("%s%sIDX:  ", prefix, OUTPUT_PREFIX);
    for (int i = 1; i < LBOUND_P2; i++) {
        printf("%d   ", i);
    }
    printf("\n");

    // Top header
    printf("%s%s    %s%s", prefix, OUTPUT_PREFIX, TL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, ET, HL);
    }
    printf("%s%s%s\n", HL, HL, TR);

    // Upper row
    printf("%s%s%s%s%s%s%s", prefix, OUTPUT_PREFIX, TL, HL, HL, HL, ER);
    for (int i = HBOUND_P2; i > LBOUND_P2; --i) {
        if (cells[i] == INT32_MIN)
            printf(" X %s", VL);
        else
            printf("%3d%s", cells[i], VL);
    }
    if (cells[LBOUND_P2] == INT32_MIN)
        printf(" X %s%s%s%s%s", EL, HL, HL, HL, TR);
    else
        printf("%3d%s%s%s%s%s", cells[LBOUND_P2], EL, HL, HL, HL, TR);

    if (color == -1) {
        printf("  %s%s", PLAYER_INDICATOR, playerDescriptor);
    }
    printf("\n");

    // Middle separator
    printf("%s%s%s%3d%s%s", prefix, OUTPUT_PREFIX, VL, cells[SCORE_P2], EL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, CR, HL);
    }
    printf("%s%s%s%3d%s\n", HL, HL, ER, cells[SCORE_P1], VL);

    // Lower row
    printf("%s%s%s%s%s%s%s", prefix, OUTPUT_PREFIX, BL, HL, HL, HL, ER);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        if (cells[i] == INT32_MIN)
            printf(" X %s", VL);
        else
            printf("%3d%s", cells[i], VL);
    }
    if (cells[HBOUND_P1] == INT32_MIN)
        printf(" X %s%s%s%s%s", EL, HL, HL, HL, BR);
    else
        printf("%3d%s%s%s%s%s", cells[HBOUND_P1], EL, HL, HL, HL, BR);

    if (color == 1) {
        printf("  %s%s", PLAYER_INDICATOR, playerDescriptor);
    }
    printf("\n");

    // Bottom footer
    printf("%s%s    %s%s", prefix, OUTPUT_PREFIX, BL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, EB, HL);
    }
    printf("%s%s%s\n", HL, HL, BR);
}

void renderBoard(const Board *board, const char* prefix, const GameSettings* settings) {
    int cells[14];
    for (int i = 0; i < 14; ++i) {
        cells[i] = board->cells[i];
    }
    renderCustomBoard(cells, board->color, prefix, settings);
}

void renderWelcome() {
    printf("+-----------------------------------------+\n");
    printf("| %sWelcome to CMancala!                 |\n", OUTPUT_PREFIX);
    printf("| %sType 'help' for a list of commands   |\n", OUTPUT_PREFIX);
    printf("|                                         |\n");
    printf("| (c) Alexander Kurtz 2026                |\n");
    printf("+-----------------------------------------+\n");
    printf("\n");
}

void renderOutput(const char* message, const char* prefix) {
    printf("%s%s%s\n", prefix, OUTPUT_PREFIX, message);
}

static void renderStatBoard(const double* cells, const char* title, const char* format, const char* prefix) {
    printf("%s%s  %s:\n", prefix, OUTPUT_PREFIX, title);

    // Top Border
    printf("%s%s  %s", prefix, OUTPUT_PREFIX, TL);
    for (int j = 0; j < 6; j++) {
        printf("%s%s%s%s%s%s%s", HL, HL, HL, HL, HL, HL, HL);
        if (j < 5) printf("%s", HL);
    }
    printf("%s\n", TR);

    // Row 2 (Opponent: Indices 12 -> 7)
    printf("%s%s  %s", prefix, OUTPUT_PREFIX, VL);
    for (int j = 12; j >= 7; j--) {
        printf(format, cells[j]);
        printf("%s", VL);
    }
    printf("\n");

    // Middle Separator
    printf("%s%s  %s", prefix, OUTPUT_PREFIX, VL);
    for (int j = 0; j < 6; j++) {
        printf("%s%s%s%s%s%s%s", HL, HL, HL, HL, HL, HL, HL);
        if (j < 5) printf("%s", CR);
        else       printf("%s", VL);
    }
    printf("\n");

    // Row 1 (Player: Indices 0 -> 5)
    printf("%s%s  %s", prefix, OUTPUT_PREFIX, VL);
    for (int j = 0; j < 6; j++) {
        printf(format, cells[j]);
        printf("%s", VL);
    }
    printf("\n");

    // Bottom Border
    printf("%s%s  %s", prefix, OUTPUT_PREFIX, BL);
    for (int j = 0; j < 6; j++) {
        printf("%s%s%s%s%s%s%s", HL, HL, HL, HL, HL, HL, HL);
        if (j < 5) printf("%s", HL);
    }
    printf("%s\n", BR);
}

void renderCacheOverview(const CacheStats* stats) {
    char message[256];
    char logBuffer[32];

    renderOutput(stats->modeStr, CHEAT_PREFIX);

    const double fillPct = (stats->cacheSize > 0) ? (double)stats->setEntries / (double)stats->cacheSize * 100.0 : 0.0;
    getLogNotation(logBuffer, stats->cacheSize);
    snprintf(message, sizeof(message), "  Cache size: %-12"PRIu64" %s (%.2f%% Used)", stats->cacheSize, logBuffer, fillPct);
    renderOutput(message, CHEAT_PREFIX);

    if (stats->hasDepth) {
        const double solvedPct = (stats->setEntries > 0) ? (double)stats->solvedEntries / (double)stats->setEntries * 100.0 : 0.0;
        getLogNotation(logBuffer, stats->solvedEntries);
        snprintf(message, sizeof(message), "  Solved:     %-12"PRIu64" %s (%.2f%% of used)", stats->solvedEntries, logBuffer, solvedPct);
        renderOutput(message, CHEAT_PREFIX);
    }

    getLogNotation(logBuffer, stats->hits);
    snprintf(message, sizeof(message), "  Hits:       %-12"PRIu64" %s", stats->hits, logBuffer);
    renderOutput(message, CHEAT_PREFIX);

    if (stats->hasDepth) {
        const uint64_t badHits = stats->hits - stats->hitsLegal;
        const double badHitPercent = (stats->hits > 0) ? (double)badHits / (double)stats->hits * 100.0 : 0.0;
        getLogNotation(logBuffer, badHits);
        snprintf(message, sizeof(message), "    Shallow:  %-12"PRIu64" %s (%.2f%%)", badHits, logBuffer, badHitPercent);
        renderOutput(message, CHEAT_PREFIX);
    }

    const double swapPct = (stats->hits > 0) ? (double)stats->lruSwaps / (double)stats->hits * 100.0 : 0.0;
    getLogNotation(logBuffer, stats->lruSwaps);
    snprintf(message, sizeof(message), "    LRU Swap: %-12"PRIu64" %s (%.2f%%)", stats->lruSwaps, logBuffer, swapPct);
    renderOutput(message, CHEAT_PREFIX);

    renderOutput("  Cache Overwrites:", CHEAT_PREFIX);
    getLogNotation(logBuffer, stats->overwriteImprove);
    snprintf(message, sizeof(message), "    Improve:  %-12"PRIu64" %s", stats->overwriteImprove, logBuffer);
    renderOutput(message, CHEAT_PREFIX);

    getLogNotation(logBuffer, stats->overwriteEvict);
    snprintf(message, sizeof(message), "    Evict:    %-12"PRIu64" %s", stats->overwriteEvict, logBuffer);
    renderOutput(message, CHEAT_PREFIX);

    if (stats->failStones > 0 || stats->failRange > 0) {
        renderOutput("  Encoding Fail Counts:", CHEAT_PREFIX);
        getLogNotation(logBuffer, stats->failStones);
        snprintf(message, sizeof(message), "    Stones:   %-12"PRIu64" %s", stats->failStones, logBuffer);
        renderOutput(message, CHEAT_PREFIX);
        getLogNotation(logBuffer, stats->failRange);
        snprintf(message, sizeof(message), "    Value:    %-12"PRIu64" %s", stats->failRange, logBuffer);
        renderOutput(message, CHEAT_PREFIX);
    }

    if (stats->setEntries > 0) {
        snprintf(message, sizeof(message), "  Bounds:     E %.2f%% | L %.2f%% | U %.2f%%",
                 (double)stats->exactCount / (double)stats->setEntries * 100.0,
                 (double)stats->lowerCount / (double)stats->setEntries * 100.0,
                 (double)stats->upperCount / (double)stats->setEntries * 100.0);
    } else {
        snprintf(message, sizeof(message), "  Bounds:     E 0.00%% | L 0.00%% | U 0.00%%");
    }
    renderOutput(message, CHEAT_PREFIX);

    if (stats->setEntries > 0) {
        renderStatBoard(stats->avgStones, "Average Stones", "%7.1f", CHEAT_PREFIX);
        renderStatBoard(stats->maxStones, "Maximum Stones", "%7.0f", CHEAT_PREFIX);
        
        // Render the two new log-scale frequency boards
        renderStatBoard(stats->over7,  "Freq > 7  (log10)", "%7.2f", CHEAT_PREFIX);
        renderStatBoard(stats->over15, "Freq > 15 (log10)", "%7.2f", CHEAT_PREFIX);
    }

    if (stats->hasDepth) {
        if (stats->nonSolvedCount > 0) {
            const double avgDepth = (double)stats->depthSum / (double)stats->nonSolvedCount;
            snprintf(message, sizeof(message), "  Depth:      avg %.2f | max %u", avgDepth, (unsigned)stats->maxDepth);
            renderOutput(message, CHEAT_PREFIX);

            const uint32_t span = (uint32_t)stats->maxDepth + 1;
            enum { DEPTH_BINS = 8 };
            const uint32_t binW = (span + DEPTH_BINS - 1) / DEPTH_BINS;

            renderOutput("  Depth range| Count        | Percent", CHEAT_PREFIX);
            renderOutput("  ------------------------------------", CHEAT_PREFIX);

            for (uint32_t bi = 0; bi < DEPTH_BINS; bi++) {
                const uint32_t start = bi * binW;
                uint32_t end = start + binW - 1;
                if (end > stats->maxDepth) end = stats->maxDepth;
                const double pct = (double)stats->depthBins[bi] / (double)stats->nonSolvedCount * 100.0;
                snprintf(message, sizeof(message), "  %3u-%-3u    | %-12" PRIu64 "| %6.2f%%", start, end, stats->depthBins[bi], pct);
                renderOutput(message, CHEAT_PREFIX);
                if (end == stats->maxDepth) break;
            }
            renderOutput("  ------------------------------------", CHEAT_PREFIX);
        } else {
            renderOutput("  Depth:      avg 0.00 | max 0", CHEAT_PREFIX);
        }
    }

    renderOutput("  Fragmentation", CHEAT_PREFIX);
    renderOutput("  Chunk Type | Start Index       | Chunk Size", CHEAT_PREFIX);
    renderOutput("  --------------------------------------------", CHEAT_PREFIX);

    for (int i = 0; i < stats->chunkCount; i++) {
        snprintf(message, sizeof(message), "     %s   | %17" PRIu64 " | %17" PRIu64,
                 stats->topChunks[i].type ? "Set  " : "Unset", stats->topChunks[i].start, stats->topChunks[i].size);
        renderOutput(message, CHEAT_PREFIX);
    }
    if (stats->chunkCount == OUTPUT_CHUNK_COUNT) renderOutput("  ...", CHEAT_PREFIX);
    renderOutput("  --------------------------------------------", CHEAT_PREFIX);
}
