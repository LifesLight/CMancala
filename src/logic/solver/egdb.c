/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/egdb.h"

// Combinatorial number system lookup: ways[s][p] = C(s+p-1, p-1)
// Gives the number of ways to distribute s stones into p pits
uint64_t ways[EGDB_MAX_STONES + 1][13];
int8_t* egdb_tables[EGDB_MAX_STONES + 1] = {NULL};
bool egdb_is_mmapped[EGDB_MAX_STONES + 1] = {false};

#ifdef LZ4
    bool egdb_is_compressed[EGDB_MAX_STONES + 1] = {false};
    uint8_t* egdb_comp_data[EGDB_MAX_STONES + 1] = {NULL};
    uint64_t* egdb_comp_offsets[EGDB_MAX_STONES + 1] = {NULL};
#endif

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

    // Base cases
    for (int p = 1; p <= 12; p++) ways[0][p] = 1;
    for (int s = 0; s <= EGDB_MAX_STONES; s++) ways[s][1] = 1;

    // Fill using Pascal's rule: ways[s][p] = ways[s][p-1] + ways[s-1][p]
    for (int p = 2; p <= 12; p++) {
        for (int s = 1; s <= EGDB_MAX_STONES; s++) {
            ways[s][p] = ways[s][p - 1] + ways[s - 1][p];
        }
    }

    initialized = true;
}

static inline uint64_t getEGDBIndex(const Board* board) {
    // Reorder cells so current player's pits come first
    uint8_t rel[12];
    if (board->color == 1) {
        for (int i = LBOUND_P1; i <= HBOUND_P1; i++) rel[i] = board->cells[i];
        for (int i = LBOUND_P2; i <= HBOUND_P2; i++) rel[i - 1] = board->cells[i];
    } else {
        for (int i = LBOUND_P2; i <= HBOUND_P2; i++) rel[i - LBOUND_P2] = board->cells[i];
        for (int i = LBOUND_P1; i <= HBOUND_P1; i++) rel[i + 6] = board->cells[i];
    }

    // Count total stones on the board
    int stones = 0;
    for (int i = 0; i < 12; i++) stones += rel[i];

    // Encode position using combinatorial number system
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

#ifdef LZ4
static bool compressBufferToRuntime(int s, int8_t* raw_data, uint64_t size) {
    uint32_t block_size = EGDB_LZ4_BLOCK_SIZE;
    uint64_t num_chunks = (size + block_size - 1) / block_size;

    // Allocate offset table for chunk boundaries
    if (egdb_comp_offsets[s]) free(egdb_comp_offsets[s]);
    egdb_comp_offsets[s] = malloc((num_chunks + 1) * sizeof(uint64_t));
    if (!egdb_comp_offsets[s]) return false;

    // Allocate worst-case temporary buffer for compressed output
    uint64_t max_dest_size = num_chunks * (uint64_t)LZ4_compressBound(block_size);
    uint8_t* temp_comp_buffer = malloc(max_dest_size);
    if (!temp_comp_buffer) {
        free(egdb_comp_offsets[s]);
        egdb_comp_offsets[s] = NULL;
        return false;
    }

    // Allocate reusable HC compression state (no streaming needed, blocks are independent)
    void* lz4State = malloc(LZ4_sizeofStateHC());

    // Compress each block independently for random access
    uint64_t current_offset = 0;
    for (uint64_t i = 0; i < num_chunks; i++) {
        egdb_comp_offsets[s][i] = current_offset;

        uint64_t start_idx = i * block_size;
        int chunk_bytes = block_size;
        if (start_idx + chunk_bytes > size) {
            chunk_bytes = (int)(size - start_idx);
        }

        int comp_bytes = LZ4_compress_HC_extStateHC(
            lz4State,
            (const char*)&raw_data[start_idx],
            (char*)&temp_comp_buffer[current_offset],
            chunk_bytes,
            LZ4_compressBound(chunk_bytes),
            LZ4HC_CLEVEL_MAX
        );

        current_offset += comp_bytes;
    }

    // Store sentinel offset marking end of last chunk
    egdb_comp_offsets[s][num_chunks] = current_offset;
    free(lz4State);

    // Shrink to actual compressed size
    if (egdb_comp_data[s]) free(egdb_comp_data[s]);
    egdb_comp_data[s] = malloc(current_offset);
    if (egdb_comp_data[s]) {
        memcpy(egdb_comp_data[s], temp_comp_buffer, current_offset);
        egdb_is_compressed[s] = true;
    } else {
        egdb_is_compressed[s] = false;
    }

    free(temp_comp_buffer);
    return egdb_is_compressed[s];
}

static void saveCompressedRuntimeToDisk(int s, uint64_t uncomp_size) {
    char lz4_filename[256];
    snprintf(lz4_filename, sizeof(lz4_filename), "EGDB/egdb_%d.lz4db", s);

    FILE* f_out = fopen(lz4_filename, "wb");
    if (!f_out) return;

    // Write file header
    uint32_t magic = EGDB_LZ4_MAGIC;
    uint32_t b_size = EGDB_LZ4_BLOCK_SIZE;
    uint64_t num_chunks = (uncomp_size + b_size - 1) / b_size;
    uint64_t comp_size = egdb_comp_offsets[s][num_chunks];

    fwrite(&magic, 4, 1, f_out);
    fwrite(&b_size, 4, 1, f_out);
    fwrite(&num_chunks, 8, 1, f_out);
    fwrite(&uncomp_size, 8, 1, f_out);

    // Write chunk offset table followed by compressed payload
    fwrite(egdb_comp_offsets[s], 8, num_chunks + 1, f_out);
    fwrite(egdb_comp_data[s], 1, comp_size, f_out);

    fclose(f_out);
}
#endif

bool EGDB_probe(Board* board, int* score) {
    egdb_probes++;

    int stonesLeft = egdb_total_stones_configured - board->cells[SCORE_P1] - board->cells[SCORE_P2];

    // Check if this stone count is in range of loaded tables
    if (stonesLeft <= 0 || stonesLeft > loaded_egdb_max_stones) {
        return false;
    }

    uint64_t idx = getEGDBIndex(board);
    int8_t futureVal;

#ifdef LZ4
    if (egdb_is_compressed[stonesLeft]) {
        // Block index and position within block (compiler emits shr/and for power-of-2)
        uint64_t chunk_id = idx / EGDB_LZ4_BLOCK_SIZE;
        uint64_t offset_in_chunk = idx % EGDB_LZ4_BLOCK_SIZE;

        uint64_t comp_offset = egdb_comp_offsets[stonesLeft][chunk_id];
        int comp_size = (int)(egdb_comp_offsets[stonesLeft][chunk_id + 1] - comp_offset);

        // Only decompress up to the byte we need, not the full block
        int target = (int)(offset_in_chunk + 1);
        int8_t temp_buf[EGDB_LZ4_BLOCK_SIZE];
        int decompressed = LZ4_decompress_safe_partial(
            (const char*)&egdb_comp_data[stonesLeft][comp_offset],
            (char*)temp_buf,
            comp_size,
            target,
            EGDB_LZ4_BLOCK_SIZE
        );

        // Validate decompression produced enough bytes
        if (decompressed < target) return false;

        futureVal = temp_buf[offset_in_chunk];
    } else
#endif
    if (egdb_tables[stonesLeft]) {
        futureVal = egdb_tables[stonesLeft][idx];
    } else {
        return false;
    }

    // Sentinel values mean no result available
    if (futureVal == EGDB_UNCOMPUTED || futureVal == EGDB_VISITING) return false;

    // Combine stored future differential with current score differential
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

    // Decode combinatorial index back into per-pit stone counts
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

    // Populate board with decoded layout
    memset(board, 0, sizeof(Board));
    board->color = 1;
    for (int i = 0; i <= 5; i++) board->cells[i] = rel[i];
    for (int i = 0; i <= 5; i++) board->cells[i + 7] = rel[i + 6];
}

static int8_t crunch(int stones, uint64_t index, Board* board) {
    // Cycle detection: already being explored in this DFS path
    if (egdb_tables[stones][index] == EGDB_VISITING) return 0;

    // Already solved
    if (egdb_tables[stones][index] != EGDB_UNCOMPUTED) return egdb_tables[stones][index];

    int best_score = -127;
    egdb_tables[stones][index] = EGDB_VISITING;
    bool can_move = false;

    // Try every legal move for the current player
    for (int i = LBOUND_P1; i <= HBOUND_P1; i++) {
        // Filter invalid moves
        if (board->cells[i] == 0) continue;
        can_move = true;

        // Make copied board with move applied
        Board next = *board;
        makeMoveOnBoardClassic(&next, i);
        processBoardTerminal(&next);

        // Count stones remaining on the board after the move
        int next_stones = 0;
        for (int p = LBOUND_P1; p <= HBOUND_P1; p++) next_stones += next.cells[p];
        for (int p = LBOUND_P2; p <= HBOUND_P2; p++) next_stones += next.cells[p];

        // Score differential gained by this move alone
        int diff_gained = (next.cells[SCORE_P1] - board->cells[SCORE_P1])
                        - (next.cells[SCORE_P2] - board->cells[SCORE_P2]);

        int score = 0;

        if (next_stones == 0) {
            // Game over: no future component
            score = diff_gained;
        } else {
            int8_t lookup;

            if (next_stones < stones) {
                // Fewer stones: look up from already-solved lower layer
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
                // Same player continues: recurse directly
                lookup = crunch(next_stones, getEGDBIndex(&next), &next);
            } else {
                // Opponent's turn: normalize board and recurse
                Board normalized;
                unhashToBoard(getEGDBIndex(&next), next_stones, &normalized);
                lookup = crunch(next_stones, getEGDBIndex(&next), &normalized);
            }

            // Negate lookup if the turn switched
            score = (next.color == board->color)
                ? (diff_gained + lookup)
                : (diff_gained - lookup);
        }

        if (score > best_score) best_score = score;
    }

    // No legal moves means a pass with zero differential
    if (!can_move) best_score = 0;

    egdb_tables[stones][index] = best_score;
    return best_score;
}

void generateEGDB(int max_stones, bool use_compression) {
    initWaysTable();
    MKDIR("EGDB");
    char msg[128];

#ifdef LZ4
    snprintf(msg, sizeof(msg), "Checking EGDB (1..%d) [%s]...",
             max_stones, use_compression ? "COMPRESSED" : "RAW");
#else
    if (use_compression) {
        renderOutput("Compression requested but not compiled. Falling back to RAW.", CONFIG_PREFIX);
        use_compression = false;
    }
    snprintf(msg, sizeof(msg), "Checking EGDB (1..%d) [RAW]...", max_stones);
#endif
    renderOutput(msg, CONFIG_PREFIX);

    for (int s = 1; s <= max_stones; s++) {
        uint64_t size = ways[s][12];
        char bin_filename[256];
        snprintf(bin_filename, sizeof(bin_filename), "EGDB/egdb_%d.bin", s);
        bool bin_exists = (access(bin_filename, F_OK) == 0);
        bool loaded = false;

        if (bin_exists) {
#ifdef LZ4
            if (use_compression) {
                // Try loading pre-compressed .lz4db file first
                char lz4_filename[256];
                snprintf(lz4_filename, sizeof(lz4_filename), "EGDB/egdb_%d.lz4db", s);

                if (access(lz4_filename, F_OK) == 0) {
                    FILE* f_lz4 = fopen(lz4_filename, "rb");
                    if (f_lz4) {
                        uint32_t magic;
                        if (fread(&magic, 4, 1, f_lz4) == 1 && magic == EGDB_LZ4_MAGIC) {
                            uint32_t f_bsize;
                            uint64_t num_chunks, uncomp_size;
                            bool ok = true;

                            if (fread(&f_bsize, 4, 1, f_lz4) != 1) ok = false;
                            if (ok && fread(&num_chunks, 8, 1, f_lz4) != 1) ok = false;
                            if (ok && fread(&uncomp_size, 8, 1, f_lz4) != 1) ok = false;

                            if (ok) {
                                egdb_comp_offsets[s] = malloc((num_chunks + 1) * sizeof(uint64_t));
                                if (fread(egdb_comp_offsets[s], 8, num_chunks + 1, f_lz4) != (num_chunks + 1)) {
                                    ok = false;
                                }
                            }

                            if (ok) {
                                uint64_t comp_size = egdb_comp_offsets[s][num_chunks];
                                egdb_comp_data[s] = malloc(comp_size);
                                if (fread(egdb_comp_data[s], 1, comp_size, f_lz4) != comp_size) {
                                    ok = false;
                                }
                            }

                            if (ok) {
                                egdb_is_compressed[s] = true;
                                loaded = true;
                            } else {
                                // Partial read failed, clean up
                                if (egdb_comp_offsets[s]) {
                                    free(egdb_comp_offsets[s]);
                                    egdb_comp_offsets[s] = NULL;
                                }
                                if (egdb_comp_data[s]) {
                                    free(egdb_comp_data[s]);
                                    egdb_comp_data[s] = NULL;
                                }
                            }
                        }
                        fclose(f_lz4);
                    }
                }

                // Fall back to compressing from raw .bin if no .lz4db found
                if (!loaded) {
                    int fd = open(bin_filename, O_RDONLY);
                    if (fd != -1) {
                        void* mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
                        close(fd);
                        if (mapped != MAP_FAILED) {
                            compressBufferToRuntime(s, (int8_t*)mapped, size);
                            munmap(mapped, size);
                            saveCompressedRuntimeToDisk(s, size);
                            loaded = true;
                        }
                    }
                }
            } else
#endif
            {
                // Load raw table via memory-mapped file
                int fd = open(bin_filename, O_RDONLY);
                if (fd != -1) {
                    void* mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
                    close(fd);
                    if (mapped != MAP_FAILED) {
                        egdb_tables[s] = (int8_t*)mapped;
                        egdb_is_mmapped[s] = true;
                        loaded = true;
                    }
                }
            }
        }

        if (!loaded) {
            // No cached file found, generate from scratch
            snprintf(msg, sizeof(msg), "Generating layer %d...", s);
            renderOutput(msg, CONFIG_PREFIX);

            egdb_tables[s] = malloc(size);
            if (!egdb_tables[s]) {
                renderOutput("OOM", CONFIG_PREFIX);
                break;
            }
            egdb_is_mmapped[s] = false;
            memset(egdb_tables[s], EGDB_UNCOMPUTED, size);

            // Solve all positions in this layer
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

            // Save raw table to disk
            FILE* f_out = fopen(bin_filename, "wb");
            if (f_out) {
                fwrite(egdb_tables[s], 1, size, f_out);
                fclose(f_out);
            }

#ifdef LZ4
            if (use_compression) {
                // Compress the generated table and save the compressed version
                if (compressBufferToRuntime(s, egdb_tables[s], size)) {
                    saveCompressedRuntimeToDisk(s, size);
                }
                free(egdb_tables[s]);
                egdb_tables[s] = NULL;
            } else
#endif
            {
                // Free raw buffer and re-open as memory-mapped file
                free(egdb_tables[s]);
                egdb_tables[s] = NULL;

                int fd = open(bin_filename, O_RDONLY);
                if (fd != -1) {
                    egdb_tables[s] = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
                    egdb_is_mmapped[s] = true;
                    close(fd);
                }
            }
        }

        loaded_egdb_max_stones = s;
    }

    renderOutput("EGDB Ready.", CONFIG_PREFIX);
}

void freeEGDB() {
    for (int s = 1; s <= loaded_egdb_max_stones; s++) {
#ifdef LZ4
        if (egdb_is_compressed[s]) {
            free(egdb_comp_data[s]);
            free(egdb_comp_offsets[s]);
            egdb_comp_data[s] = NULL;
            egdb_comp_offsets[s] = NULL;
            egdb_is_compressed[s] = false;
        }
#endif
        if (egdb_tables[s]) {
            if (egdb_is_mmapped[s]) {
                munmap(egdb_tables[s], ways[s][12]);
            } else {
                free(egdb_tables[s]);
            }
            egdb_tables[s] = NULL;
        }
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
#ifdef LZ4
        if (egdb_is_compressed[s]) {
            uint64_t num_chunks = (ways[s][12] + EGDB_LZ4_BLOCK_SIZE - 1) / EGDB_LZ4_BLOCK_SIZE;
            // Compressed payload + offset table
            *sizeBytes += egdb_comp_offsets[s][num_chunks];
            *sizeBytes += (num_chunks + 1) * sizeof(uint64_t);
        } else
#endif
        if (ways[s][12] > 0) {
            *sizeBytes += ways[s][12];
        }
    }
}

void resetEGDBStats() {
    egdb_probes = 0;
    egdb_hits = 0;
}
