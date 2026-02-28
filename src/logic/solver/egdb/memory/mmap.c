/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/egdb/core.h"
#include "logic/solver/egdb/egdb_mem.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

static bool egdb_is_mmapped[EGDB_MAX_STONES + 1] = {false};

void egdb_mem_init(void) {}

bool egdb_mem_load(int s, uint64_t size) {
    char bin_filename[256];
    snprintf(bin_filename, sizeof(bin_filename), "EGDB/egdb_%d.bin", s);

    if (access(bin_filename, F_OK) != 0) return false;

    int fd = open(bin_filename, O_RDONLY);
    if (fd != -1) {
        void* mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        if (mapped != MAP_FAILED) {
            egdb_tables[s] = (int8_t*)mapped;
            egdb_is_mmapped[s] = true;
            return true;
        }
    }
    return false;
}

void egdb_mem_alloc(int s, uint64_t size) {
    egdb_tables[s] = malloc(size);
    egdb_is_mmapped[s] = false;
}

void egdb_mem_save(int s, uint64_t size) {
    char bin_filename[256];
    snprintf(bin_filename, sizeof(bin_filename), "EGDB/egdb_%d.bin", s);

    FILE* f_out = fopen(bin_filename, "wb");
    if (f_out) {
        fwrite(egdb_tables[s], 1, size, f_out);
        fclose(f_out);
    }

    free(egdb_tables[s]);
    egdb_tables[s] = NULL;

    int fd = open(bin_filename, O_RDONLY);
    if (fd != -1) {
        egdb_tables[s] = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        egdb_is_mmapped[s] = true;
        close(fd);
    }
}

void egdb_mem_free_layer(int s, uint64_t size) {
    if (egdb_tables[s]) {
        if (egdb_is_mmapped[s]) {
            munmap(egdb_tables[s], size);
        } else {
            free(egdb_tables[s]);
        }
        egdb_tables[s] = NULL;
        egdb_is_mmapped[s] = false;
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
