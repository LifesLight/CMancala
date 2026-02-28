/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/egdb/core.h"
#include "logic/solver/egdb/egdb_mem.h"

#ifndef EGDB_BACKEND_NAME
#define EGDB_BACKEND_NAME "UNKNOWN"
#endif

uint64_t ways[EGDB_MAX_STONES + 1][13];
int8_t* egdb_tables[EGDB_MAX_STONES + 1] = {NULL};

int loaded_egdb_max_stones = 0;
int egdb_total_stones_configured = 48;
uint64_t egdb_probes = 0;
uint64_t egdb_hits = 0;

void configureStoneCountEGDB(int stonesPerPit) {
    egdb_total_stones_configured = stonesPerPit * 12;
}

static void initWaysTable() {
    static bool initialized = false;
    if (initialized) return;

    for (int p = 1; p <= 12; p++) ways[0][p] = 1;
    for (int s = 0; s <= EGDB_MAX_STONES; s++) ways[s][1] = 1;

    for (int p = 2; p <= 12; p++) {
        for (int s = 1; s <= EGDB_MAX_STONES; s++) {
            ways[s][p] = ways[s][p - 1] + ways[s - 1][p];
        }
    }

    initialized = true;
    egdb_mem_init();
}

static inline uint64_t getEGDBIndex(const Board* board) {
    uint8_t rel[12];
    if (board->color == 1) {
        for (int i = LBOUND_P1; i <= HBOUND_P1; i++) rel[i] = board->cells[i];
        for (int i = LBOUND_P2; i <= HBOUND_P2; i++) rel[i - 1] = board->cells[i];
    } else {
        for (int i = LBOUND_P2; i <= HBOUND_P2; i++) rel[i - LBOUND_P2] = board->cells[i];
        for (int i = LBOUND_P1; i <= HBOUND_P1; i++) rel[i + 6] = board->cells[i];
    }

    int stones = 0;
    for (int i = 0; i < 12; i++) stones += rel[i];

    uint64_t index = 0;
    int pitsLeft = 12;
    for (int i = 0; i < 11; i++) {
        int s = rel[i];
        for (int k = 0; k < s; k++) {
            index += ways[stones - k][pitsLeft - 1];
        }
        stones -= s;
        pitsLeft--;
        if (stones == 0) break;
    }

    return index;
}

bool EGDB_probe(Board* board, int* score) {
    egdb_probes++;

    int stonesLeft = egdb_total_stones_configured - board->cells[SCORE_P1] - board->cells[SCORE_P2];

    if (stonesLeft <= 0 || stonesLeft > loaded_egdb_max_stones) {
        return false;
    }

    uint64_t idx = getEGDBIndex(board);
    int8_t futureVal;

    if (!egdb_mem_probe(stonesLeft, idx, &futureVal)) {
        return false;
    }

    if (futureVal == EGDB_UNCOMPUTED || futureVal == EGDB_VISITING) return false;

    int currentDiff = (board->color == 1)
        ? (board->cells[SCORE_P1] - board->cells[SCORE_P2])
        : (board->cells[SCORE_P2] - board->cells[SCORE_P1]);

    *score = currentDiff + futureVal;
    egdb_hits++;
    return true;
}

static void unhashToBoard(uint64_t index, int stones, Board* board) {
    uint8_t rel[12];
    int stonesLeft = stones;
    int pitsLeft = 12;
    uint64_t temp_idx = index;

    for (int i = 0; i < 11; i++) {
        int cnt = 0;
        while (cnt <= stonesLeft) {
            uint64_t w = ways[stonesLeft - cnt][pitsLeft - 1];
            if (temp_idx < w) break;
            temp_idx -= w;
            cnt++;
        }
        rel[i] = cnt;
        stonesLeft -= cnt;
        pitsLeft--;
    }
    rel[11] = stonesLeft;

    memset(board, 0, sizeof(Board));
    board->color = 1;
    for (int i = 0; i <= 5; i++) board->cells[i] = rel[i];
    for (int i = 0; i <= 5; i++) board->cells[i + 7] = rel[i + 6];
}

static int8_t crunch(int stones, uint64_t index, Board* board) {
    if (egdb_tables[stones][index] == EGDB_VISITING) return 0;
    if (egdb_tables[stones][index] != EGDB_UNCOMPUTED) return egdb_tables[stones][index];

    int best_score = -127;
    egdb_tables[stones][index] = EGDB_VISITING;
    bool can_move = false;

    for (int i = LBOUND_P1; i <= HBOUND_P1; i++) {
        if (board->cells[i] == 0) continue;
        can_move = true;

        Board next = *board;
        makeMoveOnBoardClassic(&next, i);
        processBoardTerminal(&next);

        int next_stones = 0;
        for (int p = LBOUND_P1; p <= HBOUND_P1; p++) next_stones += next.cells[p];
        for (int p = LBOUND_P2; p <= HBOUND_P2; p++) next_stones += next.cells[p];

        int diff_gained = (next.cells[SCORE_P1] - board->cells[SCORE_P1])
                        - (next.cells[SCORE_P2] - board->cells[SCORE_P2]);

        int score = 0;

        if (next_stones == 0) {
            score = diff_gained;
        } else {
            int8_t lookup;

            if (next_stones < stones) {
                int tmp_val;
                if (EGDB_probe(&next, &tmp_val)) {
                    int cur_diff = (next.color == 1)
                        ? (next.cells[SCORE_P1] - next.cells[SCORE_P2])
                        : (next.cells[SCORE_P2] - next.cells[SCORE_P1]);
                    lookup = tmp_val - cur_diff;
                } else {
                    lookup = 0;
                }
            } else if (next.color == board->color) {
                lookup = crunch(next_stones, getEGDBIndex(&next), &next);
            } else {
                Board normalized;
                unhashToBoard(getEGDBIndex(&next), next_stones, &normalized);
                lookup = crunch(next_stones, getEGDBIndex(&next), &normalized);
            }

            score = (next.color == board->color)
                ? (diff_gained + lookup)
                : (diff_gained - lookup);
        }

        if (score > best_score) best_score = score;
    }

    if (!can_move) best_score = 0;

    egdb_tables[stones][index] = best_score;
    return best_score;
}

void generateEGDB(int max_stones) {
    initWaysTable();
    MKDIR("EGDB");
    char msg[128];

    snprintf(msg, sizeof(msg), "Checking EGDB (1..%d) [%s]...", max_stones, EGDB_BACKEND_NAME);
    renderOutput(msg, CONFIG_PREFIX);

    for (int s = 1; s <= max_stones; s++) {
        uint64_t size = ways[s][12];

        if (!egdb_mem_load(s, size)) {
            snprintf(msg, sizeof(msg), "Generating layer %d...", s);
            renderOutput(msg, CONFIG_PREFIX);

            egdb_mem_alloc(s, size);
            if (!egdb_tables[s]) {
                renderOutput("OOM", CONFIG_PREFIX);
                break;
            }
            memset(egdb_tables[s], EGDB_UNCOMPUTED, size);

            startEGDBProgress();
            uint64_t interval = size / 200;
            if (!interval) interval = 1;

            for (uint64_t idx = 0; idx < size; idx++) {
                if (idx % interval == 0) updateEGDBProgress(s, idx, size);
                if (egdb_tables[s][idx] == EGDB_UNCOMPUTED) {
                    Board b;
                    unhashToBoard(idx, s, &b);
                    crunch(s, idx, &b);
                }
            }

            updateEGDBProgress(s, size, size);
            finishEGDBProgress();

            egdb_mem_save(s, size);
        }

        loaded_egdb_max_stones = s;
    }

    renderOutput("EGDB Ready.", CONFIG_PREFIX);
}

void freeEGDB() {
    for (int s = 1; s <= loaded_egdb_max_stones; s++) {
        egdb_mem_free_layer(s, ways[s][12]);
    }
    loaded_egdb_max_stones = 0;
    resetEGDBStats();
}

void getEGDBStats(uint64_t* sizeBytes, uint64_t* probes, uint64_t* hits, int* maxStones) {
    *probes = egdb_probes;
    *hits = egdb_hits;
    *maxStones = loaded_egdb_max_stones;
    *sizeBytes = 0;

    for (int s = 1; s <= loaded_egdb_max_stones; s++) {
        *sizeBytes += egdb_mem_get_size(s, ways[s][12]);
    }
}

void resetEGDBStats() {
    egdb_probes = 0;
    egdb_hits = 0;
}
