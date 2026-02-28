/**
 * Copyright (c) Alexander Kurtz 2026
 */

#pragma once

#include "logic/solver/egdb/core.h"
#include <stdint.h>
#include <stdbool.h>

void egdb_mem_init(void);
bool egdb_mem_load(int s, uint64_t size);
void egdb_mem_alloc(int s, uint64_t size);
void egdb_mem_save(int s, uint64_t size);
void egdb_mem_free_layer(int s, uint64_t size);
bool egdb_mem_probe(int s, uint64_t idx, int8_t* val);
uint64_t egdb_mem_get_size(int s, uint64_t size_uncompressed);
