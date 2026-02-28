/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/egdb/core.h"
#include "logic/solver/egdb/egdb_mem.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void egdb_mem_init(void) {}

bool egdb_mem_load(int s, uint64_t size) {
    char bin_filename[256];
    snprintf(bin_filename, sizeof(bin_filename), "EGDB/egdb_%d.bin", s);

    if (access(bin_filename, F_OK) != 0) return false;

    FILE* f = fopen(bin_filename, "rb");
    if (!f) return false;

    egdb_tables[s] = malloc(size);
    if (!egdb_tables[s]) {
        fclose(f);
        return false;
    }

    if (fread(egdb_tables[s], 1, size, f) != size) {
        free(egdb_tables[s]);
        egdb_tables[s] = NULL;
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

void egdb_mem_alloc(int s, uint64_t size) {
    egdb_tables[s] = malloc(size);
}

void egdb_mem_save(int s, uint64_t size) {
    char bin_filename[256];
    snprintf(bin_filename, sizeof(bin_filename), "EGDB/egdb_%d.bin", s);

    FILE* f_out = fopen(bin_filename, "wb");
    if (f_out) {
        fwrite(egdb_tables[s], 1, size, f_out);
        fclose(f_out);
    }
}

void egdb_mem_free_layer(int s, uint64_t size) {
    (void)size;
    if (egdb_tables[s]) {
        free(egdb_tables[s]);
        egdb_tables[s] = NULL;
    }
}

bool egdb_mem_probe(int s, uint64_t idx, int8_t* val) {
    if (egdb_tables[s]) {
        *val = egdb_tables[s][idx];
        return true;
    }
    return false;
}

uint64_t egdb_mem_get_size(int s, uint64_t size_uncompressed) {
    if (egdb_tables[s]) return size_uncompressed;
    return 0;
}
