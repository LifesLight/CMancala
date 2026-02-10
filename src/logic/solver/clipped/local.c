/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "logic/solver/clipped/local.h"

#define PREFIX LOCAL_CLIP
#define IS_CLIPPED 1
#include "logic/solver/impl/local_template.h"
#undef PREFIX
#undef IS_CLIPPED
