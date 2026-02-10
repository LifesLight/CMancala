/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/clipped/global.h"

static bool solved;

// Template Instantiation
#define PREFIX GLOBAL_CLIP
#define IS_CLIPPED 1
#include "logic/solver/impl/global_template.h"
#undef PREFIX
#undef IS_CLIPPED
