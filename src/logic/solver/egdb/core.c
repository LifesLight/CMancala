/**
 * Copyright (c) Alexander Kurtz 2026
 * 
 * The EGDB code is strongly inspired by https://github.com/girving/kalah
 */

#include "logic/solver/egdb/core.h"


uint64_t ways[EGDB_MAX_STONES + 1][13];
int8_t* egdb_tables[EGDB_MAX_STONES + 1] = {NULL};

int loaded_egdb_max_stones = 0;
bool loaded_egdb_is_avalanche = false;
int egdb_total_stones_configured = 48;
uint64_t egdb_hits = 0;

void configureStoneCountEGDB(int totalStones) {
    egdb_total_stones_configured = totalStones;
}

static void initWaysTable() {
    static bool initialized = false;
    if (initialized) return;

    // Initialize base cases for combinations
    for (int p = 1; p <= 12; p++) ways[0][p] = 1;
    for (int s = 0; s <= EGDB_MAX_STONES; s++) ways[s][1] = 1;

    // Build the ways table iteratively
    for (int p = 2; p <= 12; p++) {
        for (int s = 1; s <= EGDB_MAX_STONES; s++) {
            ways[s][p] = ways[s][p - 1] + ways[s - 1][p];
        }
    }

    initialized = true;
    egdb_mem_init();
}

static inline uint64_t getEGDBIndex(const Board* board, int stones) {
    uint8_t rel[12];

    // 2. Fully unrolled Board Normalization (No loops here)
    if (board->color == 1) {
        rel[0] = board->cells[0];  rel[1] = board->cells[1];  rel[2] = board->cells[2];
        rel[3] = board->cells[3];  rel[4] = board->cells[4];  rel[5] = board->cells[5];
        rel[6] = board->cells[7];  rel[7] = board->cells[8];  rel[8] = board->cells[9];
        rel[9] = board->cells[10]; rel[10] = board->cells[11]; rel[11] = board->cells[12];
    } else {
        rel[0] = board->cells[7];  rel[1] = board->cells[8];  rel[2] = board->cells[9];
        rel[3] = board->cells[10]; rel[4] = board->cells[11]; rel[5] = board->cells[12];
        rel[6] = board->cells[0];  rel[7] = board->cells[1];  rel[8] = board->cells[2];
        rel[9] = board->cells[3];  rel[10] = board->cells[4]; rel[11] = board->cells[5];
    }

    uint64_t index = 0;
    int pitsLeft = 12;

    for (int i = 0; i < 11; i++) {
        int s = rel[i];
        if (s > 0) {
            index += ways[stones][pitsLeft] - ways[stones - s][pitsLeft];
            stones -= s;
            if (stones == 0) break;
        }
        pitsLeft--;
    }

    return index;
}

bool EGDB_probe(Board* board, int* score) {
    bool current_is_avalanche = (getMoveFunction() != CLASSIC_MOVE);
    if (current_is_avalanche != loaded_egdb_is_avalanche) {
        return false;
    }

    int stonesLeft = egdb_total_stones_configured - board->cells[SCORE_P1] - board->cells[SCORE_P2];

    if (stonesLeft <= 0 || stonesLeft > loaded_egdb_max_stones) {
        return false;
    }

    // 3. Pass stonesLeft down here
    uint64_t idx = getEGDBIndex(board, stonesLeft);
    int8_t futureVal;

    if (!egdb_mem_probe(stonesLeft, idx, &futureVal)) {
        return false;
    }

    if (futureVal == EGDB_UNCOMPUTED || futureVal == EGDB_VISITING) {
        return false;
    }

    // 4. Branchless Math: board->color is 1 or -1. 
    // This perfectly toggles the subtraction direction without an if/else ternary!
    int currentDiff = board->color * (board->cells[SCORE_P1] - board->cells[SCORE_P2]);

    *score = currentDiff + futureVal;
    egdb_hits++;

    return true;
}

static void unhashToBoard(uint64_t index, int stones, Board* board) {
    uint8_t rel[12];
    int stonesLeft = stones;
    int pitsLeft = 12;
    uint64_t temp_idx = index;

    // Reconstruct normalized stone distribution
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

    // Populate actual board structure
    memset(board, 0, sizeof(Board));
    board->color = 1;
    for (int i = 0; i <= 5; i++) board->cells[i] = rel[i];
    for (int i = 0; i <= 5; i++) board->cells[i + 7] = rel[i + 6];
}

static int8_t crunch(int stones, uint64_t index, Board* board) {
    // Return early if already visited or computed
    if (egdb_tables[stones][index] == EGDB_VISITING) return 0;
    if (egdb_tables[stones][index] != EGDB_UNCOMPUTED) return egdb_tables[stones][index];

    int best_score = -127;
    egdb_tables[stones][index] = EGDB_VISITING;
    bool can_move = false;

    // Iterate over all possible moves
    for (int i = LBOUND_P1; i <= HBOUND_P1; i++) {
        // Filter invalid moves
        if (board->cells[i] == 0) continue;
        can_move = true;

        // Make copied board with move made
        Board next = *board;
        if (loaded_egdb_is_avalanche) {
            makeMoveOnBoardAvalanche(&next, i);
        } else {
            makeMoveOnBoardClassic(&next, i);
        }
        processBoardTerminal(&next);

        // Count stones remaining after move
        int next_stones = 0;
        for (int p = LBOUND_P1; p <= HBOUND_P1; p++) next_stones += next.cells[p];
        for (int p = LBOUND_P2; p <= HBOUND_P2; p++) next_stones += next.cells[p];

        int diff_gained = (next.cells[SCORE_P1] - board->cells[SCORE_P1])
                        - (next.cells[SCORE_P2] - board->cells[SCORE_P2]);

        int score = 0;

        // Base case: no stones left
        if (next_stones == 0) {
            score = diff_gained;
        } else {
            int8_t lookup;

            // Probe smaller layer if stones were captured
            if (next_stones < stones) {
                // Layer next_stones is guaranteed to be fully computed already
                lookup = egdb_tables[next_stones][getEGDBIndex(&next, next_stones)];
            } else if (next.color == board->color) {
                // Next turn is same player
                lookup = crunch(next_stones, getEGDBIndex(&next, next_stones), &next);
            } else {
                // Next turn is opponent, reorient board
                Board normalized;
                unhashToBoard(getEGDBIndex(&next, next_stones), next_stones, &normalized);
                lookup = crunch(next_stones, getEGDBIndex(&next, next_stones), &normalized);
            }

            // Adjust score based on who is playing next
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

void generateEGDB(int max_stones, bool is_avalanche) {
    initWaysTable();
    loaded_egdb_is_avalanche = is_avalanche;
    MKDIR("EGDB");
    char msg[128];

    snprintf(msg, sizeof(msg), "Checking EGDB %s(1..%d) [%s]...", is_avalanche ? "Avalanche " : "", max_stones, EGDB_BACKEND_NAME);
    renderOutput(msg, CONFIG_PREFIX);

    // Build layers iteratively
    for (int s = 1; s <= max_stones; s++) {
        uint64_t size = ways[s][12];

        // If not found on disk, we must compute it
        if (!egdb_mem_load(s, size, is_avalanche)) {
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

            // Crunch all states in current layer
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

            egdb_mem_save(s, size, is_avalanche);
        }

        loaded_egdb_max_stones = s;
    }
}

void freeEGDB() {
    for (int s = 1; s <= loaded_egdb_max_stones; s++) {
        if (egdb_tables[s] || egdb_mem_get_size(s, ways[s][12]) > 0) {
            egdb_mem_free_layer(s, ways[s][12]);
            egdb_tables[s] = NULL;
        }
    }

    loaded_egdb_max_stones = 0;
    loaded_egdb_is_avalanche = false;
    resetEGDBStats();
}

void getEGDBStats(uint64_t* sizeBytes, uint64_t* hits, int* minStones, int* maxStones) {
    *hits = egdb_hits;
    *minStones = 1;
    *maxStones = loaded_egdb_max_stones;
    *sizeBytes = 0;

    // Accumulate actual memory footprint using backend size checker
    for (int s = 1; s <= loaded_egdb_max_stones; s++) {
        *sizeBytes += egdb_mem_get_size(s, ways[s][12]);
    }
}

void resetEGDBStats() {
    egdb_hits = 0;
}
