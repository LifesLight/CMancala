/**
 * Copyright (c) Alexander Kurtz 2026
 */

 #include "logic/solver/local.h"

static bool lastUsedClip = false;

#define PREFIX LOCAL
#define IS_CLIPPED 0
#include "logic/solver/impl/local_template.h"
#undef PREFIX
#undef IS_CLIPPED

#define PREFIX LOCAL_CLIP
#define IS_CLIPPED 1
#include "logic/solver/impl/local_template.h"
#undef PREFIX
#undef IS_CLIPPED
