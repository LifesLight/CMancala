/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "user/render.h"

static clock_t progressStartTime;
static const SolverConfig* progressConfig = NULL;
static const char* progressPrefix = NULL;
static const int BAR_WIDTH = 40;
static bool progressFirstUpdate;

static void formatNodeCount(uint64_t nodes, char* buffer, size_t size) {
    if (nodes < 1000) {
        snprintf(buffer, size, "%" PRIu64, nodes);
    } else if (nodes < 1000000) {
        snprintf(buffer, size, "%.1fk", (double)nodes / 1000.0);
    } else if (nodes < 1000000000) {
        snprintf(buffer, size, "%.2fM", (double)nodes / 1000000.0);
    } else {
        snprintf(buffer, size, "%.2fG", (double)nodes / 1000000000.0);
    }
}

static void formatTimeDuration(double seconds, char* buffer, size_t size) {
    if (seconds < 60.0) {
        snprintf(buffer, size, "%.2fs", seconds);
    } else if (seconds < 3600.0) {
        int min = (int)(seconds / 60);
        int sec = (int)(seconds) % 60;
        snprintf(buffer, size, "%dm %ds", min, sec);
    } else if (seconds < 86400.0) {
        int hr = (int)(seconds / 3600);
        int min = (int)((seconds - (hr * 3600)) / 60);
        snprintf(buffer, size, "%dh %dm", hr, min);
    } else {
        int day = (int)(seconds / 86400);
        int hr = (int)((seconds - (day * 86400)) / 3600);
        snprintf(buffer, size, "%dd %dh", day, hr);
    }
}

void startProgress(const SolverConfig* config, const char* prefix) {
    if (!config->progressBar) return;
    progressConfig = config;
    progressPrefix = prefix ? prefix : "";
    progressStartTime = clock();
    progressFirstUpdate = true;
}

void updateProgress(int currentDepth, int bestMove, int score, uint64_t nodeCount) {
    if (!progressConfig) return;

    double elapsed = (double)(clock() - progressStartTime) / CLOCKS_PER_SEC;

    // Bar tracks whichever active limit is closest to terminating the search
    double percentage = 0.0;
    bool hasLimit = false;

    if (progressConfig->depth > 0) {
        double dp = (double)currentDepth / (double)progressConfig->depth;
        if (dp > percentage) percentage = dp;
        hasLimit = true;
    }
    if (progressConfig->timeLimit > 0) {
        double tp = elapsed / progressConfig->timeLimit;
        if (tp > percentage) percentage = tp;
        hasLimit = true;
    }
    if (!hasLimit) percentage = 1.0;

    if (percentage > 1.0) percentage = 1.0;
    if (percentage < 0.0) percentage = 0.0;

    // Cursor: move up one line to overwrite both lines
    if (!progressFirstUpdate) {
        printf("\033[A\r");
    }
    progressFirstUpdate = false;

    // --- Line 1: Progress bar ---
    int filledLen = (int)(percentage * BAR_WIDTH);

    printf("%s>> " BAR_CAP_L, progressPrefix);
    for (int i = 0; i < BAR_WIDTH; i++) {
        if (i < filledLen) printf(BAR_FILL);
        else               printf(BAR_EMPTY);
    }
    printf(BAR_CAP_R " %3d%%\033[K\n", (int)(percentage * 100));

    // --- Line 2: Stats  D, T, E, M, N ---
    printf("%s>>  ", progressPrefix);

    // D: depth
    if (progressConfig->depth > 0)
        printf("D:%d/%d", currentDepth, progressConfig->depth);
    else
        printf("D:%d", currentDepth);

    printf(" " STAT_SEP " ");

    // T: time
    char timeStr[32];
    formatTimeDuration(elapsed, timeStr, sizeof(timeStr));
    if (progressConfig->timeLimit > 0) {
        char limitStr[32];
        formatTimeDuration(progressConfig->timeLimit, limitStr, sizeof(limitStr));
        printf("T:%s/%s", timeStr, limitStr);
    } else {
        printf("T:%s", timeStr);
    }

    printf(" " STAT_SEP " ");

    // E: eval
    if (progressConfig->clip) {
        if (score > 0)       printf("E: WIN");
        else if (score < 0)  printf("E: LOSS");
        else                  printf("E: DRAW");
    } else {
        printf("E:%+d", score);
    }

    printf(" " STAT_SEP " ");

    if (bestMove > 6) {
        bestMove = 13 - bestMove;
    }

    // M: move
    printf("M:%d", bestMove);

    printf(" " STAT_SEP " ");

    // N: nodes
    char nodeStr[32];
    formatNodeCount(nodeCount, nodeStr, sizeof(nodeStr));
    printf("N:%s   ", nodeStr);

    printf("\033[K");
    fflush(stdout);
}

void finishProgress() {
    if (!progressConfig) return;
    progressConfig = NULL;
    printf("\n");
}

void startEGDBProgress() {
    progressStartTime = clock();
    progressFirstUpdate = true;
    progressPrefix = CONFIG_PREFIX; 
}

void updateEGDBProgress(int stones, uint64_t current, uint64_t total) {
    double elapsed = (double)(clock() - progressStartTime) / CLOCKS_PER_SEC;
    double percentage = (total > 0) ? (double)current / (double)total : 0.0;

    if (percentage > 1.0) percentage = 1.0;

    if (!progressFirstUpdate) {
        printf("\033[A\r");
    }
    progressFirstUpdate = false;

    int filledLen = (int)(percentage * BAR_WIDTH);

    printf("%s>> " BAR_CAP_L, progressPrefix);
    for (int i = 0; i < BAR_WIDTH; i++) {
        if (i < filledLen) printf(BAR_FILL);
        else               printf(BAR_EMPTY);
    }
    printf(BAR_CAP_R " %3d%%\033[K\n", (int)(percentage * 100));

    printf("%s>>  ", progressPrefix);

    printf("S:%d", stones);
    printf(" " STAT_SEP " ");

    char timeStr[32];
    formatTimeDuration(elapsed, timeStr, sizeof(timeStr));
    printf("T:%s ", timeStr);

    printf("\033[K");
    fflush(stdout);
}

void finishEGDBProgress() {
    printf("\n");
}

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
    printf("| %sWelcome to CMancala v%s!            |\n", OUTPUT_PREFIX, MANCALA_VERSION);
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

    printf("%s%s  %s", prefix, OUTPUT_PREFIX, TL);
    for (int j = 0; j < 6; j++) {
        printf("%s%s%s%s%s%s%s", HL, HL, HL, HL, HL, HL, HL);
        if (j < 5) printf("%s", HL);
    }
    printf("%s\n", TR);

    printf("%s%s  %s", prefix, OUTPUT_PREFIX, VL);
    for (int j = 12; j >= 7; j--) {
        printf(format, cells[j]);
        printf("%s", VL);
    }
    printf("\n");

    printf("%s%s  %s", prefix, OUTPUT_PREFIX, VL);
    for (int j = 0; j < 6; j++) {
        printf("%s%s%s%s%s%s%s", HL, HL, HL, HL, HL, HL, HL);
        if (j < 5) printf("%s", CR);
        else       printf("%s", VL);
    }
    printf("\n");

    printf("%s%s  %s", prefix, OUTPUT_PREFIX, VL);
    for (int j = 0; j < 6; j++) {
        printf(format, cells[j]);
        printf("%s", VL);
    }
    printf("\n");

    printf("%s%s  %s", prefix, OUTPUT_PREFIX, BL);
    for (int j = 0; j < 6; j++) {
        printf("%s%s%s%s%s%s%s", HL, HL, HL, HL, HL, HL, HL);
        if (j < 5) printf("%s", HL);
    }
    printf("%s\n", BR);
}

void renderCacheOverview(const CacheStats* stats, bool showFrag, bool showStoneDist, bool showDepthDist) {
    char message[256];
    char logBuffer[32];

    renderOutput(stats->modeStr, CHEAT_PREFIX);

    const double fillPct = (stats->cacheSize > 0) ? (double)stats->setEntries / (double)stats->cacheSize * 100.0 : 0.0;
    getLogNotation(logBuffer, stats->cacheSize);
    snprintf(message, sizeof(message), "  Cache size: %-12"PRIu64" %s (%.2f%% Used)", stats->cacheSize, logBuffer, fillPct);
    renderOutput(message, CHEAT_PREFIX);

    double cacheMB = ((double)stats->cacheSize * stats->entrySize) / 1048576.0;
    snprintf(message, sizeof(message), "  Size (MB):  %7.2f MB", cacheMB);
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

    if (showStoneDist && stats->setEntries > 0) {
        renderStatBoard(stats->avgStones, "Average Stones", "%7.1f", CHEAT_PREFIX);
        renderStatBoard(stats->maxStones, "Maximum Stones", "%7.0f", CHEAT_PREFIX);
        renderStatBoard(stats->over7,  "Freq > 7  (log10)", "%7.2f", CHEAT_PREFIX);
        renderStatBoard(stats->over15, "Freq > 15 (log10)", "%7.2f", CHEAT_PREFIX);
    }

    if (stats->hasDepth) {
        if (stats->nonSolvedCount > 0) {
            const double avgDepth = (double)stats->depthSum / (double)stats->nonSolvedCount;
            snprintf(message, sizeof(message), "  Depth:      avg %.2f | max %u", avgDepth, (unsigned)stats->maxDepth);
            renderOutput(message, CHEAT_PREFIX);

            if (showDepthDist) {
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
            }
        } else {
            renderOutput("  Depth:      avg 0.00 | max 0", CHEAT_PREFIX);
        }
    }

    if (showFrag) {
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
}

void renderEGDBOverview() {
    uint64_t egdbProbes, egdbHits, egdbSize;
    int egdbMax;
    getEGDBStats(&egdbSize, &egdbProbes, &egdbHits, &egdbMax);

    if (egdbMax <= 0) {
        renderOutput("  EGDB not loaded or disabled", CHEAT_PREFIX);
        return;
    }

    char message[256];
    char logBuffer[32];

    snprintf(message, sizeof(message), "  EGDB Status: Loaded (%d stones max)", egdbMax);
    renderOutput(message, CHEAT_PREFIX);

    if (egdbSize < 1024) snprintf(message, sizeof(message), "    Size:     %" PRIu64 " Bytes", egdbSize);
    else if (egdbSize < 1048576) snprintf(message, sizeof(message), "    Size:     %.2f KB", (double)egdbSize/1024.0);
    else snprintf(message, sizeof(message), "    Size:     %.2f MB", (double)egdbSize/1048576.0);
    renderOutput(message, CHEAT_PREFIX);

    getLogNotation(logBuffer, egdbProbes);
    snprintf(message, sizeof(message), "    Probes:   %-12" PRIu64 " %s", egdbProbes, logBuffer);
    renderOutput(message, CHEAT_PREFIX);

    double hitRate = (egdbProbes > 0) ? ((double)egdbHits / egdbProbes * 100.0) : 0.0;
    getLogNotation(logBuffer, egdbHits);
    snprintf(message, sizeof(message), "    Hits:     %-12" PRIu64 " %s (%.2f%%)", egdbHits, logBuffer, hitRate);
    renderOutput(message, CHEAT_PREFIX);
}
