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
#include <zstd.h>
#include <zdict.h>

#define EGDB_ZSTD_MAGIC 0x4454535A
#define EGDB_ZSTD_BLOCK_SIZE 256
#define EGDB_ZSTD_DICT_CAPACITY (110 * 1024) 

// Global arrays to hold EGDB data
static bool egdb_is_compressed[EGDB_MAX_STONES + 1] = {false};
static uint8_t* egdb_comp_data[EGDB_MAX_STONES + 1] = {NULL};
static uint64_t* egdb_comp_offsets[EGDB_MAX_STONES + 1] = {NULL};

// Dictionary storage
// We need the raw dictionary content to save to disk, and the DDict for runtime speed
static void* egdb_zstd_raw_dict[EGDB_MAX_STONES + 1] = {NULL};
static size_t egdb_zstd_raw_dict_size[EGDB_MAX_STONES + 1] = {0};
static ZSTD_DDict* egdb_zstd_ddict[EGDB_MAX_STONES + 1] = {NULL};

// Single shared decompression context for single-threaded application
static ZSTD_DCtx* g_dctx = NULL;

void egdb_mem_init(void) {
    // Initialize the shared decompression context once
    g_dctx = ZSTD_createDCtx();
}

// Helper to cleanup specific layer
static void free_layer_internals(int s) {
    if (egdb_comp_data[s]) { free(egdb_comp_data[s]); egdb_comp_data[s] = NULL; }
    if (egdb_comp_offsets[s]) { free(egdb_comp_offsets[s]); egdb_comp_offsets[s] = NULL; }
    
    if (egdb_zstd_ddict[s]) { ZSTD_freeDDict(egdb_zstd_ddict[s]); egdb_zstd_ddict[s] = NULL; }
    if (egdb_zstd_raw_dict[s]) { free(egdb_zstd_raw_dict[s]); egdb_zstd_raw_dict[s] = NULL; }
    
    egdb_zstd_raw_dict_size[s] = 0;
    egdb_is_compressed[s] = false;
}

static bool compressBufferToRuntime(int s, int8_t* raw_data, uint64_t size) {
    uint32_t block_size = EGDB_ZSTD_BLOCK_SIZE;
    uint64_t num_chunks = (size + block_size - 1) / block_size;

    // Allocate offset array
    egdb_comp_offsets[s] = malloc((num_chunks + 1) * sizeof(uint64_t));
    if (!egdb_comp_offsets[s]) return false;

    // Temporary compression buffer (conservative estimate)
    uint64_t max_dest_size = num_chunks * (uint64_t)ZSTD_compressBound(block_size);
    uint8_t* temp_comp_buffer = malloc(max_dest_size);
    if (!temp_comp_buffer) {
        free(egdb_comp_offsets[s]);
        egdb_comp_offsets[s] = NULL;
        return false;
    }

    // --- 1. Dictionary Training ---
    // Use up to ~100MB of samples or all chunks if smaller
    size_t nb_samples = (num_chunks > 100000) ? 100000 : num_chunks;
    size_t* sample_sizes = malloc(nb_samples * sizeof(size_t));
    
    for (size_t i = 0; i < nb_samples; i++) {
        // Handle last chunk size potentially being smaller
        uint64_t chunk_idx = i * (num_chunks / nb_samples); 
        if (chunk_idx >= num_chunks) chunk_idx = num_chunks - 1;
        
        uint64_t start_idx = chunk_idx * block_size;
        size_t current_chunk_size = block_size;
        if (start_idx + current_chunk_size > size) {
            current_chunk_size = size - start_idx;
        }
        sample_sizes[i] = current_chunk_size;
    }

    void* dict_buf = malloc(EGDB_ZSTD_DICT_CAPACITY);
    ZSTD_CDict* cdict = NULL;

    // Train the dictionary
    size_t dict_res = ZDICT_trainFromBuffer(dict_buf, EGDB_ZSTD_DICT_CAPACITY, raw_data, sample_sizes, nb_samples);
    free(sample_sizes);

    if (!ZDICT_isError(dict_res) && dict_res > 0) {
        egdb_zstd_raw_dict[s] = dict_buf;
        egdb_zstd_raw_dict_size[s] = dict_res;
        
        // Create Compression Dict (Level 22)
        cdict = ZSTD_createCDict(dict_buf, dict_res, 22);
        
        // Create Decompression Dict (for runtime usage)
        egdb_zstd_ddict[s] = ZSTD_createDDict(dict_buf, dict_res);
    } else {
        free(dict_buf); // Training failed or not enough data, proceed without dict
        egdb_zstd_raw_dict[s] = NULL;
    }

    // --- 2. Compression Loop ---
    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    uint64_t current_offset = 0;

    for (uint64_t i = 0; i < num_chunks; i++) {
        egdb_comp_offsets[s][i] = current_offset;

        uint64_t start_idx = i * block_size;
        size_t chunk_bytes = block_size;
        if (start_idx + chunk_bytes > size) {
            chunk_bytes = size - start_idx;
        }

        size_t comp_bytes;
        if (cdict) {
            comp_bytes = ZSTD_compress_usingCDict(
                cctx,
                temp_comp_buffer + current_offset,
                ZSTD_compressBound(chunk_bytes),
                raw_data + start_idx,
                chunk_bytes,
                cdict
            );
        } else {
            comp_bytes = ZSTD_compressCCtx(
                cctx,
                temp_comp_buffer + current_offset,
                ZSTD_compressBound(chunk_bytes),
                raw_data + start_idx,
                chunk_bytes,
                22
            );
        }

        if (ZSTD_isError(comp_bytes)) {
            // Fatal compression error
            ZSTD_freeCCtx(cctx);
            if (cdict) ZSTD_freeCDict(cdict);
            free(temp_comp_buffer);
            free_layer_internals(s);
            return false;
        }

        current_offset += comp_bytes;
    }

    egdb_comp_offsets[s][num_chunks] = current_offset; // End marker

    // Cleanup Compression Resources
    ZSTD_freeCCtx(cctx);
    if (cdict) ZSTD_freeCDict(cdict);

    // Finalize Memory
    if (egdb_comp_data[s]) free(egdb_comp_data[s]);
    egdb_comp_data[s] = malloc(current_offset);
    
    if (egdb_comp_data[s]) {
        memcpy(egdb_comp_data[s], temp_comp_buffer, current_offset);
        egdb_is_compressed[s] = true;
    } else {
        free_layer_internals(s);
    }

    free(temp_comp_buffer);
    return egdb_is_compressed[s];
}

static void saveCompressedRuntimeToDisk(int s, uint64_t uncomp_size) {
    char zstd_filename[256];
    snprintf(zstd_filename, sizeof(zstd_filename), "EGDB/egdb_%d.zstddb", s);

    FILE* f_out = fopen(zstd_filename, "wb");
    if (!f_out) return;

    uint32_t magic = EGDB_ZSTD_MAGIC;
    uint32_t b_size = EGDB_ZSTD_BLOCK_SIZE;
    uint64_t num_chunks = (uncomp_size + b_size - 1) / b_size;
    uint64_t comp_size = egdb_comp_offsets[s][num_chunks];
    uint64_t dict_size = egdb_zstd_raw_dict_size[s];

    // Header
    fwrite(&magic, 4, 1, f_out);
    fwrite(&b_size, 4, 1, f_out);
    fwrite(&num_chunks, 8, 1, f_out);
    fwrite(&uncomp_size, 8, 1, f_out);
    fwrite(&dict_size, 8, 1, f_out);

    // Dictionary Blob
    if (dict_size > 0 && egdb_zstd_raw_dict[s]) {
        fwrite(egdb_zstd_raw_dict[s], 1, dict_size, f_out);
    }

    // Offsets
    fwrite(egdb_comp_offsets[s], 8, num_chunks + 1, f_out);
    
    // Compressed Data
    fwrite(egdb_comp_data[s], 1, comp_size, f_out);

    fclose(f_out);
}

bool egdb_mem_load(int s, uint64_t size) {
    char zstd_filename[256];
    snprintf(zstd_filename, sizeof(zstd_filename), "EGDB/egdb_%d.zstddb", s);

    // 1. Try Loading Compressed File
    if (access(zstd_filename, F_OK) == 0) {
        FILE* f = fopen(zstd_filename, "rb");
        if (f) {
            uint32_t magic, f_bsize;
            uint64_t num_chunks, uncomp_size, dict_size;
            bool ok = true;

            if (fread(&magic, 4, 1, f) != 1 || magic != EGDB_ZSTD_MAGIC) ok = false;
            if (ok && fread(&f_bsize, 4, 1, f) != 1) ok = false;
            if (ok && fread(&num_chunks, 8, 1, f) != 1) ok = false;
            if (ok && fread(&uncomp_size, 8, 1, f) != 1) ok = false;
            if (ok && fread(&dict_size, 8, 1, f) != 1) ok = false;

            // Load Dictionary
            if (ok && dict_size > 0) {
                egdb_zstd_raw_dict[s] = malloc(dict_size);
                if (fread(egdb_zstd_raw_dict[s], 1, dict_size, f) == dict_size) {
                    egdb_zstd_raw_dict_size[s] = dict_size;
                    // Create DDict immediately for fast lookups
                    egdb_zstd_ddict[s] = ZSTD_createDDict(egdb_zstd_raw_dict[s], dict_size);
                } else {
                    ok = false;
                }
            }

            // Load Offsets
            if (ok) {
                egdb_comp_offsets[s] = malloc((num_chunks + 1) * sizeof(uint64_t));
                if (fread(egdb_comp_offsets[s], 8, num_chunks + 1, f) != (num_chunks + 1)) {
                    ok = false;
                }
            }

            // Load Compressed Data
            if (ok) {
                uint64_t comp_size = egdb_comp_offsets[s][num_chunks];
                egdb_comp_data[s] = malloc(comp_size);
                if (fread(egdb_comp_data[s], 1, comp_size, f) != comp_size) {
                    ok = false;
                }
            }

            if (ok) {
                egdb_is_compressed[s] = true;
                fclose(f);
                return true;
            }

            // Cleanup on failure
            free_layer_internals(s);
            fclose(f);
        }
    }

    // 2. Fallback: Load Raw Binary -> Compress -> Save
    char bin_filename[256];
    snprintf(bin_filename, sizeof(bin_filename), "EGDB/egdb_%d.bin", s);
    if (access(bin_filename, F_OK) == 0) {
        int fd = open(bin_filename, O_RDONLY);
        if (fd != -1) {
            void* mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
            close(fd);
            if (mapped != MAP_FAILED) {
                bool result = compressBufferToRuntime(s, (int8_t*)mapped, size);
                munmap(mapped, size);
                if (result) {
                    saveCompressedRuntimeToDisk(s, size);
                    return true;
                }
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
    // Save Raw
    char bin_filename[256];
    snprintf(bin_filename, sizeof(bin_filename), "EGDB/egdb_%d.bin", s);

    FILE* f_out = fopen(bin_filename, "wb");
    if (f_out) {
        fwrite(egdb_tables[s], 1, size, f_out);
        fclose(f_out);
    }

    // Compress in memory and save .zstddb
    if (compressBufferToRuntime(s, egdb_tables[s], size)) {
        saveCompressedRuntimeToDisk(s, size);
    }
    
    // Release raw memory
    free(egdb_tables[s]);
    egdb_tables[s] = NULL;
}

void egdb_mem_free_layer(int s, uint64_t size) {
    (void)size;
    if (egdb_is_compressed[s]) {
        free_layer_internals(s);
    } else if (egdb_tables[s]) {
        free(egdb_tables[s]);
        egdb_tables[s] = NULL;
    }
}

// Hot path
bool egdb_mem_probe(int s, uint64_t idx, int8_t* val) {
    if (egdb_is_compressed[s]) {
        // Init the global context lazily if not done in init (safety check)
        if (!g_dctx) g_dctx = ZSTD_createDCtx();

        uint64_t chunk_id = idx / EGDB_ZSTD_BLOCK_SIZE;
        uint64_t offset_in_chunk = idx % EGDB_ZSTD_BLOCK_SIZE;

        uint64_t comp_offset = egdb_comp_offsets[s][chunk_id];
        size_t comp_size = (size_t)(egdb_comp_offsets[s][chunk_id + 1] - comp_offset);

        // Zstd requires the full destination buffer size to match expectations
        // or using streaming/partial which is slower. 
        // 1KB is small enough to put on stack.
        int8_t temp_buf[EGDB_ZSTD_BLOCK_SIZE]; 
        
        size_t decompressed;

        if (egdb_zstd_ddict[s]) {
            // Fast path: Decompress using digested dictionary
            decompressed = ZSTD_decompress_usingDDict(
                g_dctx,
                temp_buf, 
                EGDB_ZSTD_BLOCK_SIZE, 
                egdb_comp_data[s] + comp_offset, 
                comp_size,
                egdb_zstd_ddict[s]
            );
        } else {
            // Fallback (rare, only if dict training failed)
            decompressed = ZSTD_decompressDCtx(
                g_dctx,
                temp_buf,
                EGDB_ZSTD_BLOCK_SIZE,
                egdb_comp_data[s] + comp_offset,
                comp_size
            );
        }

        // Check for error or if the requested index is beyond the decompressed data
        // (This happens in the very last block if it's smaller than 1024)
        if (ZSTD_isError(decompressed) || offset_in_chunk >= decompressed) {
            return false;
        }

        *val = temp_buf[offset_in_chunk];
        return true;
    } 
    else if (egdb_tables[s]) {
        *val = egdb_tables[s][idx];
        return true;
    }
    return false;
}

uint64_t egdb_mem_get_size(int s, uint64_t size_uncompressed) {
    if (egdb_is_compressed[s]) {
        uint64_t num_chunks = (size_uncompressed + EGDB_ZSTD_BLOCK_SIZE - 1) / EGDB_ZSTD_BLOCK_SIZE;
        uint64_t meta_size = (num_chunks + 1) * sizeof(uint64_t); // Offsets
        uint64_t data_size = egdb_comp_offsets[s][num_chunks];
        uint64_t dict_size = egdb_zstd_raw_dict_size[s];
        
        // Note: DDict size is opaque, but we count the raw dict size
        return data_size + meta_size + dict_size;
    } else if (egdb_tables[s]) {
        return size_uncompressed;
    }
    return 0;
}
