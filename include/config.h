#pragma once

/**
 * Copyright (c) Alexander Kurtz 2024
 */

#ifndef _WIN32
/**
 * Allows encoding of boards
 * Requires 128 bit integers
*/
#define ENCODING
#endif

/**
 * Tracks the throughput of the last simulation
*/
#define TRACK_THROUGHPUT

/**
 * Enables greedy solving
 * Greedy solving accepts any solution that is solved and above a certain threshold
 * This will lead to a move trace that ends the game in a winning fashion with a minimum of moves
 * This is not a perfect solution as we will only win by GOOD_ENOUGH but it drastically reduces the search space
*/
// #define GREEDY_SOLVING

#ifdef GREEDY_SOLVING
/**
 * Specify how which evaluation is good enough for the solver
 * We will early terminate when a guranteed win with this eval is found
*/
#define GOOD_ENOUGH 1
#endif

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
