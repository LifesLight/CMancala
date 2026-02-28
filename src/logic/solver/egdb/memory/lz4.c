/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/egdb/core.h"
#include "logic/solver/egdb/egdb_mem.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <lz4.h>
#include <lz4hc.h>

#define EGDB_LZ4_MAGIC 0x48435A4C
#define EGDB_LZ4_BLOCK_SIZE 1024

static bool egdb_is_compressed[EGDB_MAX_STONES + 1] = {false};
static uint8_t* egdb_comp_data[EGDB_MAX_STONES + 1] = {NULL};
static uint64_t* egdb_comp_offsets[EGDB_MAX_STONES + 1] = {NULL};

void egdb_mem_init(void) {}

static bool compressBufferToRuntime(int s, int8_t* raw_data, uint64_t size) {
    uint32_t block_size = EGDB_LZ4_BLOCK_SIZE;
    uint64_t num_chunks = (size + block_size - 1) / block_size;

    if (egdb_comp_offsets[s]) free(egdb_comp_offsets[s]);
    egdb_comp_offsets[s] = malloc((num_chunks + 1) * sizeof(uint64_t));
    if (!egdb_comp_offsets[s]) return false;

    uint64_t max_dest_size = num_chunks * (uint64_t)LZ4_compressBound(block_size);
    uint8_t* temp_comp_buffer = malloc(max_dest_size);
    if (!temp_comp_buffer) {
        free(egdb_comp_offsets[s]);
        egdb_comp_offsets[s] = NULL;
        return false;
    }

    void* lz4State = malloc(LZ4_sizeofStateHC());

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

    egdb_comp_offsets[s][num_chunks] = current_offset;
    free(lz4State);

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

    uint32_t magic = EGDB_LZ4_MAGIC;
    uint32_t b_size = EGDB_LZ4_BLOCK_SIZE;
    uint64_t num_chunks = (uncomp_size + b_size - 1) / b_size;
    uint64_t comp_size = egdb_comp_offsets[s][num_chunks];

    fwrite(&magic, 4, 1, f_out);
    fwrite(&b_size, 4, 1, f_out);
    fwrite(&num_chunks, 8, 1, f_out);
    fwrite(&uncomp_size, 8, 1, f_out);

    fwrite(egdb_comp_offsets[s], 8, num_chunks + 1, f_out);
    fwrite(egdb_comp_data[s], 1, comp_size, f_out);

    fclose(f_out);
}

bool egdb_mem_load(int s, uint64_t size) {
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
                    fclose(f_lz4);
                    return true;
                } else {
                    if (egdb_comp_offsets[s]) { free(egdb_comp_offsets[s]); egdb_comp_offsets[s] = NULL; }
                    if (egdb_comp_data[s]) { free(egdb_comp_data[s]); egdb_comp_data[s] = NULL; }
                }
            }
            fclose(f_lz4);
        }
    }

    // Fallback: If only raw .bin exists, load, compress, save to lz4db
    char bin_filename[256];
    snprintf(bin_filename, sizeof(bin_filename), "EGDB/egdb_%d.bin", s);
    if (access(bin_filename, F_OK) == 0) {
        int fd = open(bin_filename, O_RDONLY);
        if (fd != -1) {
            void* mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
            close(fd);
            if (mapped != MAP_FAILED) {
                compressBufferToRuntime(s, (int8_t*)mapped, size);
                munmap(mapped, size);
                saveCompressedRuntimeToDisk(s, size);
                return true;
            }
        }
    }

    return false;
}

void egdb_mem_alloc(int s, uint64_t size) {
    egdb_tables[s] = malloc(size);
    egdb_is_compressed[s] = false;
}

void egdb_mem_save(int s, uint64_t size) {
    // Write out raw binary first
    char bin_filename[256];
    snprintf(bin_filename, sizeof(bin_filename), "EGDB/egdb_%d.bin", s);

    FILE* f_out = fopen(bin_filename, "wb");
    if (f_out) {
        fwrite(egdb_tables[s], 1, size, f_out);
        fclose(f_out);
    }

    if (compressBufferToRuntime(s, egdb_tables[s], size)) {
        saveCompressedRuntimeToDisk(s, size);
    }
    
    free(egdb_tables[s]);
    egdb_tables[s] = NULL;
}

void egdb_mem_free_layer(int s, uint64_t size) {
    (void)size;
    if (egdb_is_compressed[s]) {
        free(egdb_comp_data[s]);
        free(egdb_comp_offsets[s]);
        egdb_comp_data[s] = NULL;
        egdb_comp_offsets[s] = NULL;
        egdb_is_compressed[s] = false;
    }
}

bool egdb_mem_probe(int s, uint64_t idx, int8_t* val) {
    if (egdb_is_compressed[s]) {
        uint64_t chunk_id = idx / EGDB_LZ4_BLOCK_SIZE;
        uint64_t offset_in_chunk = idx % EGDB_LZ4_BLOCK_SIZE;

        uint64_t comp_offset = egdb_comp_offsets[s][chunk_id];
        int comp_size = (int)(egdb_comp_offsets[s][chunk_id + 1] - comp_offset);

        int target = (int)(offset_in_chunk + 1);
        int8_t temp_buf[EGDB_LZ4_BLOCK_SIZE];
        int decompressed = LZ4_decompress_safe_partial(
            (const char*)&egdb_comp_data[s][comp_offset],
            (char*)temp_buf,
            comp_size,
            target,
            EGDB_LZ4_BLOCK_SIZE
        );

        if (decompressed < target) return false;
        *val = temp_buf[offset_in_chunk];
        return true;
    } else if (egdb_tables[s]) {
        *val = egdb_tables[s][idx];
        return true;
    }
    return false;
}

uint64_t egdb_mem_get_size(int s, uint64_t size_uncompressed) {
    if (egdb_is_compressed[s]) {
        uint64_t num_chunks = (size_uncompressed + EGDB_LZ4_BLOCK_SIZE - 1) / EGDB_LZ4_BLOCK_SIZE;
        return egdb_comp_offsets[s][num_chunks] + (num_chunks + 1) * sizeof(uint64_t);
    } else if (egdb_tables[s]) {
        return size_uncompressed;
    }
    return 0;
}
