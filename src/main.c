/**
 * Copyright (c) Alexander Kurtz 2023
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * Board is constructed like this:
 * Score cells are 6 and 13
 * Indecies P1: 0 - 5
 * Indecies P2: 7 - 12
 */

typedef uint8_t u8_t;
typedef int32_t i32_t;
typedef int64_t i64_t;

// Index of the score cells for each player
#define SCORE_P1 6
#define SCORE_P2 13


// Construct board at pointer with specified cell values
void newBoardCustomStones(u8_t *cells, bool *turn, i32_t stones) {
    // Bounds checking
    if (stones * 12 > UINT8_MAX) {
        printf("[WARNING]: Reducing %d stones per cell to %d to avoid u8_t overflow\n", stones, UINT8_MAX / 12);
        stones = UINT8_MAX / 12;
    }

    // Assign start values
    for (i32_t i = 0; i < 14; i++) {
        cells[i] = stones;
    }

    // Overwrite score cells to 0
    cells[SCORE_P1] = 0;
    cells[SCORE_P2] = 0;

    // Assign starting player
    (*turn) = true;
}

// Construct board at pointer with default values
void newBoard(u8_t *cells, bool *turn) {
    newBoardCustomStones(cells, turn, 4);
}

// Copies board to target pointer
void copyBoard(const u8_t *cells, u8_t *cellsCopy) {
    // Copy values
    memcpy(cellsCopy, cells, 14 * sizeof(u8_t));
}

// Needs to be even number otherwise rounding down
// Stones is total stones in game
void randomizeCells(u8_t *cells, i32_t stones) {
    // Since we are double assigning per 
    stones = stones / 2;

    // Check for u8_t bounds
    if (stones > UINT8_MAX / 2) {
        printf("[WARNING]: Reducing %d totoal stones to %d to avoid u8_t overflow\n", stones * 2, UINT8_MAX / 2 * 2);
        stones = UINT8_MAX / 2;
    }

    // Reset board to 0
    memset(cells, 0, 14 * sizeof(u8_t));

    // Assign mirrored random stones
    srand(time(NULL));
    for (i32_t i = 0; i < stones; i++) {
        i32_t index = rand() % 6;
        cells[index] += 1;
        cells[index + SCORE_P1 + 1] += 1;
    }
}

// Renders representation of cell
void renderBoard(const u8_t *cells) {
    // Print indecies
    printf("IDX:  ");
    for (i32_t i = 1; i < 7; i++) {
        printf("%d   ", i);
    }
    printf("\n");
    printf("    ┌─");
    for (i32_t i = 0; i < 5; ++i) {
        printf("──┬─");
    }
    printf("──┐\n");

    printf("┌───┤");
    for (i32_t i = 12; i > 7; --i) {
        printf("%3d│", cells[i]);
    }
    printf("%3d├───┐\n", cells[7]);

    printf("│%3d├─", cells[SCORE_P2]);
    for (i32_t i = 0; i < 5; ++i) {
        printf("──┼─");
    }
    printf("──┤%3d│\n", cells[SCORE_P1]);

    printf("└───┤");
    for (i32_t i = 0; i < 5; ++i) {
        printf("%3d│", cells[i]);
    }
    printf("%3d├───┘\n", cells[5]);

    printf("    └─");
    for (i32_t i = 0; i < 5; ++i) {
        printf("──┴─");
    }
    printf("──┘\n");
}

// Returns relative evaluation
i32_t getBoardEvaluation(const u8_t *cells) {
    // Calculate score delta
    return cells[SCORE_P2] - cells[SCORE_P1];
}

// Check if player ones side is empty
bool isBoardPlayerOneEmpty(const u8_t *cells) {
    // Casting the array to a single i64_t,
    // mask out the last 2 indecies and if that i64_t is 0 the side is empty
    return !(*(i64_t*)cells & 0x0000FFFFFFFFFFFF);
}

// Check if player twos side is empty
bool isBoardPlayerTwoEmpty(const u8_t *cells) {
    // Same logic as ^ just offset the pointer to check the other side
    return !(*(i64_t*)(cells + 7) & 0x0000FFFFFFFFFFFF);
}

// Returns wether the game finished
bool processBoardTerminal(u8_t *cells) {
    // Check for finish
    if (isBoardPlayerOneEmpty(cells)) {
        // Add up opposing players stones to non empty players score
        for (i32_t i = 7; i < 13; i++) {
            cells[SCORE_P2] += cells[i];
            cells[i] = 0;
        }
        return true;
    }

    if (isBoardPlayerTwoEmpty(cells)) {
        for (i32_t i = 0; i < 6; i++) {
            cells[SCORE_P1] += cells[i];
            cells[i] = 0;
        }
        return true;
    }

    return false;
}

// Performs move on the board, the provided turn bool is updated accordingly
void makeMoveOnBoard(u8_t *cells, bool *turn, i32_t actionIndex) {
    // Propagate stones
    const u8_t stones = cells[actionIndex];
    cells[actionIndex] = 0;

    // Get blocked index for this player
    const i32_t blockedIndex = (*turn) ? SCORE_P2 : SCORE_P1;

    // Index is the current working index
    i32_t index;

    // Propagate stones
    for(i32_t i = actionIndex + 1; i < actionIndex + stones + 1; i++) {
        // Modulo to wrap around board
        index = i % 14;
        if (index != blockedIndex) {
            // Add stone to cell
            cells[index]++;
        } else {
            // Offset the loop by one to make up for the blocked index
            actionIndex++;
        }
    }

    // Working with index after propagation

    // Check if last stone was placed on score field
    // If yes return without inverting turn
    if ((index == SCORE_P1 && *turn) || (index == SCORE_P2 && !*turn)) {
        return;
    }

    // Check if last stone was placed on empty field
    // If yes "Steal" stones
    if (cells[index] == 1) {
        const i32_t targetIndex = 12 - index;
        // Make sure there even are stones on the other side
        const u8_t targetValue = cells[targetIndex];

        // If there are stones on the other side steal them
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

// Make move but also automatically handles terminality
void makeMoveManual(u8_t *cells, bool *turn, i32_t index) {
    makeMoveOnBoard(cells, turn, index);
    processBoardTerminal(cells);
}

// Min
i32_t min(i32_t a, i32_t b) {
    return (a < b) ? a : b;
}

// Max
i32_t max(i32_t a, i32_t b) {
    return (a > b) ? a : b;
}

// Standard Minimax implementation
i32_t minimax(u8_t *cells, const bool turn, i32_t alpha, i32_t beta, const i32_t depth) {
    // Check if we are at terminal state
    // If yes add up all remaining stones and return
    // Otherwise force terminal with depth
    if (processBoardTerminal(cells) || depth == 0) {
        return getBoardEvaluation(cells);
    }

    // Needed for remaining code
    u8_t cellsCopy[14];
    bool newTurn;
    i32_t reference;
    i32_t i;

    // Seperation by whos turn it is
    if (turn) {
        reference = INT32_MAX;
        // Going backwards is way faster with alpha beta pruning
        for (i = 5; i >= 0; i--) {
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
        for (i = 12; i >= 7; i--) {
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


// Root call to minimax, saves best move so we can get a move and not just a eval
void minimaxRoot(u8_t *cells, bool turn, i32_t* move, i32_t* evaluation, i32_t depth) {
    // Needed for remaining code
    i32_t alpha = INT32_MIN;
    i32_t beta = INT32_MAX;
    i32_t reference;

    if (turn) {
        reference = INT32_MAX;
        for (i32_t i = 5; i >= 0; i--) {
            // Filter invalid moves
            if (cells[i] == 0) {
                continue;
            }
            // Make copied board with move made
            bool newTurn = turn;
            u8_t cellsCopy[14];
            copyBoard(cells, cellsCopy);
            makeMoveOnBoard(cellsCopy, &newTurn, i);
            i32_t eval = minimax(cellsCopy, newTurn, alpha, beta, depth - 1);
            if (eval < reference) {
                *move = i;
                reference = eval;
            }
            beta = min(reference, beta);
        }
    } else {
        // Same logic as above just for other player optimization
        reference = INT32_MIN;
        for (i32_t i = 12; i >= 7; i--) {
            if (cells[i] == 0) {
                continue;
            }
            bool newTurn = turn;
            u8_t cellsCopy[14];
            copyBoard(cells, cellsCopy);
            makeMoveOnBoard(cellsCopy, &newTurn, i);
            i32_t eval = minimax(cellsCopy, newTurn, alpha, beta, depth - 1);
            if (eval > reference) {
                *move = i;
                reference = eval;
            }
            alpha = max(reference, alpha);
        }
    }

    // "Return" evaluation
    *evaluation = reference;
}


/**
 * Main function
 * Make modifications to search depth, board layout... here
*/
i32_t main(i32_t argc, char const* argv[]) {
    // "Main" board and turn
    u8_t cells[14];
    bool turn = true;

    /**
     * Search depth for ai
     * Can also just write number into minimaxRoot param for ai vs ai with different depth...
    */
    const int aiDepth = 18;

    /**
     * Initialize board here
     * Choose from the following functions or write your own init
     * just make sure that total stones are < 256
    */
    newBoard(cells, &turn);
    //newBoardCustomStones(cells, &turn, 3);
    //randomizeCells(cells, 60);


    /**
     * Inital rendering of board
    */
    i32_t index;
    i32_t eval = 0;
    renderBoard(cells);
    printf("Turn: %s\n", turn ? "P1" : "P2");

    /**
     * Game loop
     * Make modifications for human vs human, human vs ai, ai vs ai here
    */
    while (!(isBoardPlayerOneEmpty(cells) || isBoardPlayerTwoEmpty(cells))) {
        // Check if human or ai turn
        if (turn) {
            printf("Enter move: ");
            // Catch non integer input
            if (scanf("%d", &index) != 1) {
                printf("Invalid move\n");
                // Clear input buffer
                while (getchar() != '\n');
                continue;
            }
            // Check bounds
            if (index < 1 || index > 6 || cells[index - 1] == 0) {
                printf("Invalid move\n");
                continue;
            }
            // Translate to 0 start index
            index -= 1;
        } else {
            // Call ai
            minimaxRoot(cells, turn, &index, &eval, aiDepth);
        }

        // Perform move
        makeMoveManual(cells, &turn, index);
        renderBoard(cells);
        printf("Turn: %s; Evaluation: %d;\n", turn ? "P1" : "P2", -eval);
    }

    return 0;
}
