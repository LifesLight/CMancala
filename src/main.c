/**
 * Copyright Alexander Kurtz 2023
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

/*
 * Board is constructed like this:
 * Score cells are 6 and 13
 * Indecies P1: 0 - 5
 * Indecies P2: 7 - 12
 */

typedef uint8_t u8;
typedef int32_t i32;
typedef int64_t i64;

#define SCORE_P1 6
#define SCORE_P2 13

// Data for threads
typedef struct {
    u8 *cells;
    bool turn;
    i32 evaluation;
    i32 depth;
    i32 move;
} ThreadArgs;

// Construct board at pointer with specified cell values
void newBoardCustomStones(u8 *cells, bool *turn, i32 stones) {
    // Bounds checking
    if (stones * 12 > UINT8_MAX) {
        printf("[WARNING]: Reducing %d stones per cell to %d to avoid u8 overflow\n", stones, UINT8_MAX / 12);
        stones = UINT8_MAX / 12;
    }

    // Assign start values
    for (i32 i = 0; i < 14; i++) {
        cells[i] = stones;
    }
    cells[SCORE_P1] = 0;
    cells[SCORE_P2] = 0;

    // Assign starting player
    (*turn) = true;
}

// Construct board at pointer with default values
void newBoard(u8 *cells, bool *turn) {
    newBoardCustomStones(cells, turn, 4);
}

void copyBoard(const u8 *cells, u8 *cellsCopy) {
    // Copy values
    memcpy(cellsCopy, cells, 14 * sizeof(u8));
}

// Needs to be even number otherwise rounding down
// Stones is total stones in game
void randomizeCells(u8 *cells, i32 stones) {
    // Since we are double assigning per 
    stones = stones / 2;

    // Check for u8 bounds
    if (stones > UINT8_MAX / 2) {
        printf("[WARNING]: Reducing %d totoal stones to %d to avoid u8 overflow\n", stones * 2, UINT8_MAX / 2 * 2);
        stones = UINT8_MAX / 2;
    }

    // Reset board to 0
    memset(cells, 0, 14 * sizeof(u8));

    // Assign mirrored random stones
    srand(time(NULL));
    for (i32 i = 0; i < stones; i++) {
        i32 index = rand() % 6;
        cells[index] += 1;
        cells[index + SCORE_P1 + 1] += 1;
    }
}

// Just dont question it
void renderBoard(const u8 *cells) {
    printf("    ┌─");
    for (i32 i = 0; i < 5; ++i) {
        printf("──┬─");
    }
    printf("──┐\n");

    printf("┌───┤");
    for (i32 i = 12; i > 7; --i) {
        printf("%3d│", cells[i]);
    }
    printf("%3d├───┐\n", cells[7]);

    printf("│%3d├─", cells[SCORE_P2]);
    for (i32 i = 0; i < 5; ++i) {
        printf("──┼─");
    }
    printf("──┤%3d│\n", cells[SCORE_P1]);

    printf("└───┤");
    for (i32 i = 0; i < 5; ++i) {
        printf("%3d│", cells[i]);
    }
    printf("%3d├───┘\n", cells[5]);

    printf("    └─");
    for (i32 i = 0; i < 5; ++i) {
        printf("──┴─");
    }
    printf("──┘\n");
}

// Returns relative evaluation
i32 getBoardEvaluation(const u8 *cells) {
    return cells[SCORE_P2] - cells[SCORE_P1];
}

// Check if player ones side is empty
bool isBoardPlayerOneEmpty(const u8 *cells) {
    // Casting the array to a single i64,
    // mask out the last 2 indecies and if that i64 is 0 the side is empty
    return !(*(i64*)cells & 0x0000FFFFFFFFFFFF);
}

// Check if player twos side is empty
bool isBoardPlayerTwoEmpty(const u8 *cells) {
    // Same logic as ^ just offset the pointer to check the other side
    return !(*(i64*)(cells + 7) & 0x0000FFFFFFFFFFFF);
}

// Returns wether the game finished
bool processBoardTerminal(u8 *cells) {
    // Check for finish
    if (isBoardPlayerOneEmpty(cells)) {
        for (i32 i = 7; i < 13; i++) {
            cells[SCORE_P2] += cells[i];
            cells[i] = 0;
        }
        return true;
    }

    if (isBoardPlayerTwoEmpty(cells)) {
        for (i32 i = 0; i < 6; i++) {
            cells[SCORE_P1] += cells[i];
            cells[i] = 0;
        }
        return true;
    }

    return false;
}

void makeMoveOnBoard(u8 *cells, bool *turn, i32 index) {
    // Propagate stones
    const u8 stones = cells[index];
    cells[index] = 0;

    // Calculate blocked index
    i32 blockedIndex = (*turn) ? SCORE_P2 : SCORE_P1;

    // Propagate stones
    i32 modItterator;
    for(i32 i = index + 1; i < index + stones + 1; i++) {
        modItterator = i % 14;
        if (modItterator != blockedIndex) {
            cells[modItterator]++;
        } else {
            index++;
        }
    }
    // Assign final index correctly
    index = modItterator;

    // Check if last stone was placed on score field
    // If yes return without inverting turn
    if ((index == SCORE_P1 && *turn) || (index == SCORE_P2 && !*turn)) {
        return;
    }

    // Check if last stone was placed on empty field
    // If yes "Steal" stones
    if (cells[index] == 1) {
        i32 targetIndex = 12 - index;
        // Make sure there even are stones on the other side
        u8 targetValue = cells[targetIndex];
        if (targetValue != 0) {
            // If player 2 made move
            if (!*turn && index > SCORE_P1) {
                cells[SCORE_P2] += targetValue + 1;
                cells[targetIndex] = 0;
                cells[index] = 0;
            // Player 1 made move
            } else if (*turn && index < SCORE_P1) {
                cells[SCORE_P1] += cells[targetIndex] + 1;
                cells[targetIndex] = 0;
                cells[index] = 0;
            }
        }
    }

    // Return no extra move
    *turn = !*turn;
}

void makeMoveManual(u8 *cells, bool *turn, i32 index) {
    makeMoveOnBoard(cells, turn, index);
    processBoardTerminal(cells);
}

void renderBoardWithNextMove(const u8 *cells, const bool turn) {
    renderBoard(cells);
    printf("Move: %d\n", turn);
}

// Min
i32 min(i32 a, i32 b) {
    return (a < b) ? a : b;
}

// Max
i32 max(i32 a, i32 b) {
    return (a > b) ? a : b;
}

// Standard Minimax implementation
i32 minimax(u8 *restrict cells, const bool turn, i32 alpha, i32 beta, const i32 depth) {
    // Check if we are at terminal state
    // If yes add up all remaining stones and return
    // Otherwise force terminal with depth
    if (processBoardTerminal(cells) || depth == 0) {
        return getBoardEvaluation(cells);
    }

    // Needed for remaining code
    u8 cellsCopy[14];
    bool newTurn;
    i32 reference;
    i32 i;

    // Seperation by whos turn it is
    if (turn) {
        reference = INT32_MAX;
        for (i = 0; i < 6; i++) {
            // Filter invalid moves
            if (cells[i] == 0) {
                continue;
            }
            // Make copied board with move made
            newTurn = turn;
            copyBoard(cells, cellsCopy);
            makeMoveOnBoard(cellsCopy, &newTurn, i);
            // Recursively call function to get eval and compare, optimizing for small values
            reference = min(reference, minimax(cellsCopy, newTurn, alpha, beta, depth - 1));
            // If better eval exists discard this branch since player before will never allow this position
            if (reference <= alpha) {
                break;
            }
            beta = min(reference, beta);
        }
    } else {
        // Same logic as above just for other player optimization
        reference = INT32_MIN;
        for (i = 7; i < 13; i++) {
            if (cells[i] == 0) {
                continue;
            }
            newTurn = turn;
            copyBoard(cells, cellsCopy);
            makeMoveOnBoard(cellsCopy, &newTurn, i);
            reference = max(reference, minimax(cellsCopy, newTurn, alpha, beta, depth - 1));
            if (reference >= beta) {
                break;
            }
            alpha = max(reference, alpha);
        }
    }

    // Return board evaluation
    return reference;
}

/**
 * Threading code, we just seperate the possible moves on different threads to improve performance
*/

// Worker function
void* minimaxThreadFunc(void *vargp) {
    ThreadArgs *args = (ThreadArgs*)vargp;

    u8 cellsCopy[14];
    bool newTurn = args->turn;
    copyBoard(args->cells, cellsCopy);
    makeMoveOnBoard(cellsCopy, &newTurn, args->move);
    args->evaluation = minimax(cellsCopy, newTurn, INT32_MIN, INT32_MAX, args->depth - 1);

    return NULL;
}

// We start one thread per possible move and compare them after
void minimaxRoot(u8 *cells, bool turn, i32* move, i32* evaluation, i32 depth) {
    ThreadArgs threadArgs[6];

    i32 moveOffset = turn ? 0 : 7;
    pthread_t threadIDs[6];

    // Create threads
    for (i32 i = 0; i < 6; i++) {
        threadArgs[i].cells = cells;
        threadArgs[i].turn = turn;
        threadArgs[i].depth = depth;
        threadArgs[i].evaluation = turn ? INT32_MAX : INT32_MIN;
        threadArgs[i].move = i + moveOffset;

        if (cells[threadArgs[i].move] == 0) {
            continue;
        }

        pthread_create(&threadIDs[i], NULL, minimaxThreadFunc, &threadArgs[i]);
    }

    // Join threads
    for (i32 i = 0; i < 6; i++) {
        pthread_join(threadIDs[i], NULL);
    }

    // Compare results
    i32 bestValue;
    i32 bestIndex;
    if (turn) {
        bestValue = INT32_MAX;
        for (i32 i = 0; i < 6; i++) {
            ThreadArgs args = threadArgs[i];
            if (args.evaluation < bestValue) {
                bestIndex = args.move;
                bestValue = args.evaluation;
            }
        }
    } else {
        bestValue = INT32_MIN;
        for (i32 i = 0; i < 6; i++) {
            ThreadArgs args = threadArgs[i];
            if (args.evaluation > bestValue) {
                bestIndex = args.move;
                bestValue = args.evaluation;
            }
        }
    }

    // Assign best results
    *move = bestIndex;
    *evaluation = bestValue;
}

i32 main(i32 argc, char const* argv[]) {
    u8 cells[14];
    bool turn = true;

    newBoard(cells, &turn);
    //newBoardCustomStones(cells, &turn, 6);
    //randomizeCells(cells, 60);

    i32 index;
    i32 eval;
    renderBoardWithNextMove(cells, turn);
    while (!(isBoardPlayerOneEmpty(cells) || isBoardPlayerTwoEmpty(cells))) {
        minimaxRoot(cells, turn, &index, &eval, 14);
        makeMoveManual(cells, &turn, index);
        renderBoardWithNextMove(cells, turn);
        printf("Eval: %d\n", -eval);
    }

    return 0;
}
