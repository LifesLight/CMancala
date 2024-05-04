#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */


/*
 * Board is constructed like this:
 * Score cells are 6 and 13
 * Indicies P1: 0 - 5
 * Indicies P2: 7 - 12
 */
#define LBOUND_P1 0
#define HBOUND_P1 5

#define LBOUND_P2 7
#define HBOUND_P2 12

#define SCORE_P1 6
#define SCORE_P2 13

#define ASIZE 14

#define OUTPUT_PREFIX    ">> "
#define INPUT_PREFIX     "<< "
#define CONFIG_PREFIX    "(C) "
#define CHEAT_PREFIX     "(G) "
#define PLAY_PREFIX      "(P) "
#define PLAYER_INDICATOR "<-"
