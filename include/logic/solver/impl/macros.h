/**
 * Copyright (c) Alexander Kurtz 2026
 */

#pragma once

#define TOKEN_PASTE(x, y) x ## _ ## y
#define CAT(x, y) TOKEN_PASTE(x, y)

#define FN(name) CAT(name, PREFIX)

#define PACK_VALUE(eval, bt) ((int16_t)(((eval) << 2) | ((bt) & 0x3)))
#define UNPACK_VALUE(val) ((int16_t)((val) >> 2))
#define UNPACK_BOUND(val) ((val) & 0x3)
