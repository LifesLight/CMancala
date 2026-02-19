/**
 * Copyright (c) Alexander Kurtz 2026
 */

#ifndef TEMPL_MACROS
#define TEMPL_MACROS

#define TOKEN_PASTE(x, y) x ## _ ## y
#define CAT(x, y) TOKEN_PASTE(x, y)

#define FN(name) CAT(name, PREFIX)

#endif
