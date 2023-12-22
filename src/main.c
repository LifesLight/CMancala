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
 * Indicies P1: 0 - 5
 * Indicies P2: 7 - 12
 */

typedef uint8_t u8_t;
typedef int8_t i8_t;
typedef int32_t i32_t;
typedef int64_t i64_t;

// Index of the score cells for each player
#define SCORE_P1 6
#define SCORE_P2 13

#ifdef _WIN32
// Windows cant render unicode by default
const char* HL = "-";
const char* VL = "|";
const char* TL = "+";
const char* TR = "+";
const char* BL = "+";
const char* BR = "+";
const char* EL = "+";
const char* ER = "+";
const char* ET = "+";
const char* EB = "+";
const char* CR = "+";
#else
// Unicode characters for rendering
const char* HL = "─";  // Horizontal Line
const char* VL = "│";  // Vertical Line
const char* TL = "┌";  // Top Left Corner
const char* TR = "┐";  // Top Right Corner
const char* BL = "└";  // Bottom Left Corner
const char* BR = "┘";  // Bottom Right Corner
const char* EL = "├";  // Edge Left
const char* ER = "┤";  // Edge Right
const char* ET = "┬";  // Edge Top
const char* EB = "┴";  // Edge Bottom
const char* CR = "┼";  // Cross
#endif

typedef struct 
{
    u8_t cells[14];
    i8_t color;
} Board;


// Copies board to target pointer
void copyBoard(const Board *board, Board *target) {
    memcpy(target, board, sizeof(Board));
}

// Construct board at pointer with specified cell values
void newBoardCustomStones(Board *board, i32_t stones) {
    // Bounds checking
    if (stones * 12 > UINT8_MAX) {
        printf("[WARNING]: Reducing %d stones per cell to %d to avoid u8_t overflow\n", stones, UINT8_MAX / 12);
        stones = UINT8_MAX / 12;
    }

    // Assign start values
    for (i32_t i = 0; i < 14; i++) {
        board->cells[i] = stones;
    }

    // Overwrite score cells to 0
    board->cells[SCORE_P1] = 0;
    board->cells[SCORE_P2] = 0;

    // Assign starting player
    board->color = 1;
}

// Construct board at pointer with default values
void newBoard(Board *board) {
    newBoardCustomStones(board, 4);
}

// Needs to be even number otherwise rounding down
// Stones is total stones in game
void randomizeCells(Board *board, i32_t stones) {
    // Since we are double assigning per 
    stones = stones / 2;

    // Check for u8_t bounds
    if (stones > UINT8_MAX / 2) {
        printf("[WARNING]: Reducing %d total stones to %d to avoid u8_t overflow\n", stones * 2, UINT8_MAX / 2 * 2);
        stones = UINT8_MAX / 2;
    }

    // Reset board to 0
    memset(board->cells, 0, sizeof(board->cells));

    // Assign mirrored random stones
    srand(time(NULL));
    for (i32_t i = 0; i < stones; i++) {
        i32_t index = rand() % 6;
        board->cells[index] += 1;
        board->cells[index + SCORE_P1 + 1] += 1;
    }
}

void renderBoard(const Board *board) {
    // Print indices
    printf("IDX:  ");
    for (i32_t i = 1; i < 7; i++) {
        printf("%d   ", i);
    }
    printf("\n");

    // Top header
    printf("    %s%s", TL, HL);
    for (i32_t i = 0; i < 5; ++i) {
        printf("%s%s%s%s", HL, HL, ET, HL);
    }
    printf("%s%s%s\n", HL, HL, TR);

    // Upper row
    printf("%s%s%s%s%s", TL, HL, HL, HL, ER);
    for (i32_t i = 12; i > 7; --i) {
        printf("%3d%s", board->cells[i], VL);
    }
    printf("%3d%s%s%s%s%s\n", board->cells[7], EL, HL, HL, HL, TR);

    // Middle separator
    printf("%s%3d%s%s", VL, board->cells[SCORE_P2], EL, HL);
    for (i32_t i = 0; i < 5; ++i) {
        printf("%s%s%s%s", HL, HL, CR, HL);
    }
    printf("%s%s%s%3d%s\n", HL, HL, ER, board->cells[SCORE_P1], VL);

    // Lower row
    printf("%s%s%s%s%s", BL, HL, HL, HL, ER);
    for (i32_t i = 0; i < 5; ++i) {
        printf("%3d%s", board->cells[i], VL);
    }
    printf("%3d%s%s%s%s%s\n", board->cells[5], EL, HL, HL, HL, BR);

    // Bottom footer
    printf("    %s%s", BL, HL);
    for (i32_t i = 0; i < 5; ++i) {
        printf("%s%s%s%s", HL, HL, EB, HL);
    }
    printf("%s%s%s\n", HL, HL, BR);
}

// Returns relative evaluation
i32_t getBoardEvaluation(const Board *board) {
    // Calculate score delta
    return board->cells[SCORE_P1] - board->cells[SCORE_P2];
}

// Check if player ones side is empty
bool isBoardPlayerOneEmpty(const Board *board) {
    // Casting the array to a single i64_t,
    // mask out the last 2 indices and if that i64_t is 0 the side is empty
    return !(*(i64_t*)board->cells & 0x0000FFFFFFFFFFFF);
}

// Check if player twos side is empty
bool isBoardPlayerTwoEmpty(const Board *board) {
    // Same logic as ^ just offset the pointer to check the other side
    // Also slightly different mask to check from other side (array bounds)
    return !(*(i64_t*)(board->cells + 5) & 0xFFFFFFFFFFFF0000);
}

// Returns wether the game finished
bool processBoardTerminal(Board *board) {
    // Check for finish
    if (isBoardPlayerOneEmpty(board)) {
        // Add up opposing players stones to non empty players score
        for (i32_t i = 7; i < 13; i++) {
            board->cells[SCORE_P2] += board->cells[i];
            board->cells[i] = 0;
        }
        return true;
    }

    if (isBoardPlayerTwoEmpty(board)) {
        for (i32_t i = 0; i < 6; i++) {
            board->cells[SCORE_P1] += board->cells[i];
            board->cells[i] = 0;
        }
        return true;
    }

    return false;
}

// Performs move on the board, the provided turn bool is updated accordingly
void makeMoveOnBoard(Board *board, const u8_t actionIndex) {
    // Propagate stones
    const u8_t stones = board->cells[actionIndex];
    board->cells[actionIndex] = 0;

    const bool turn = board->color == 1;

    // Get blocked index for this player
    const u8_t blockedIndex = turn ? (SCORE_P2 - 1) : (SCORE_P1 - 1);
    u8_t index = actionIndex;

    // Propagate stones
    for (u8_t i = 0; i < stones; i++) {
        // Skip blocked index
        if (index == blockedIndex) {
            index += 2;
        } else {
            // If not blocked, normal increment
            index++;
        }

        // Wrap around bounds
        if (index > 13) {
            index = index - 14;
        }

        // Increment cell
        board->cells[index]++;
    }

    // Check if last stone was placed on score field
    // If yes return without inverting turn
    if ((index == SCORE_P1 && turn) || (index == SCORE_P2 && !turn)) {
        return;
    }

    // Check if last stone was placed on empty field
    // If yes "Steal" stones
    if (board->cells[index] == 1) {
        const i32_t targetIndex = 12 - index;
        // Make sure there even are stones on the other side
        const u8_t targetValue = board->cells[targetIndex];

        // If there are stones on the other side steal them
        if (targetValue != 0) {
            // If player 2 made move
            if (!turn && index > SCORE_P1) {
                board->cells[SCORE_P2] += targetValue + 1;
                board->cells[targetIndex] = 0;
                board->cells[index] = 0;
            // Player 1 made move
            } else if (turn && index < SCORE_P1) {
                board->cells[SCORE_P1] += board->cells[targetIndex] + 1;
                board->cells[targetIndex] = 0;
                board->cells[index] = 0;
            }
        }
    }

    // Return no extra move
    board->color = -board->color;
}

// Make move but also automatically handles terminal state
void makeMoveManual(Board* board, i32_t index) {
    makeMoveOnBoard(board, index);
    processBoardTerminal(board);
}

// Min
i32_t min(i32_t a, i32_t b) {
    return (a < b) ? a : b;
}

// Max
i32_t max(i32_t a, i32_t b) {
    return (a > b) ? a : b;
}

i32_t negamax(Board *board, i32_t alpha, i32_t beta, const i32_t depth) {
    // Terminally check
    if (processBoardTerminal(board) || depth == 0) {
        return board->color * getBoardEvaluation(board);
    }

    i32_t reference = INT32_MIN;
    i32_t score;

    const i8_t start = (board->color == 1) ? 5 : 12;
    const i8_t end = (board->color == 1) ? 0 : 7;

    Board boardCopy;

    for (i8_t i = start; i >= end; i--) {
        // Filter invalid moves
        if (board->cells[i] == 0) {
            continue;
        }
        // Make copied board with move made
        copyBoard(board, &boardCopy);
        makeMoveOnBoard(&boardCopy, i);

        // Recursively call function to get eval and compare, optimizing for small values
        if (board->color == boardCopy.color)
            score = negamax(&boardCopy, alpha, beta, depth - 1);
        else
            score = -negamax(&boardCopy, -beta, -alpha, depth - 1);

        reference = max(reference, score);

        alpha = max(alpha, reference);

        if (alpha >= beta) {
            break;
        }
    }

    return reference;
}

void negamaxRoot(Board *board, i32_t *move, i32_t *evaluation, i32_t depth) {
    i32_t alpha = INT32_MIN + 1;
    // +1 to avoid INT32_MIN overflow
    i32_t beta = INT32_MAX;

    i32_t score;
    i32_t bestScore = INT32_MIN;
    i32_t bestMove = -1;
    Board boardCopy;

    const i8_t start = (board->color == 1) ? 5 : 12;
    const i8_t end = (board->color == 1) ? 0 : 7;

    for (int i = start; i >= end; --i) {
        // Filter invalid moves
        if (board->cells[i] == 0) {
            continue;
        }

        // Make copied board with move made
        copyBoard(board, &boardCopy);
        makeMoveOnBoard(&boardCopy, i);

        // Recursively call function to get eval and compare, optimizing for small values
        if (board->color == boardCopy.color)
            score = negamax(&boardCopy, alpha, beta, depth - 1);
        else
            score = -negamax(&boardCopy, -beta, -alpha, depth - 1);

        // Update best move
        if (score > bestScore) {
            bestScore = score;
            bestMove = i;
        }

        // Update alpha
        alpha = max(alpha, bestScore);
    }

    *move = bestMove;
    *evaluation = bestScore;
}


/**
 * Main function
 * Make modifications to search depth, board layout... here
*/
i32_t main(i32_t argc, char const* argv[]) {
    // "Main" board and turn
    Board board;

    /**
     * Search depth for ai
     * Can also just write number into minimaxRoot param for ai vs ai with different depth...
    */
    const int aiDepth = 22;

    /**
     * Initialize board here
     * Choose from the following functions or write your own init
     * just make sure that total stones are < 256
    */
    newBoard(&board);
    //newBoardCustomStones(cells, &turn, 3);
    //randomizeCells(cells, 60);

     /**
     * Inital rendering of board
    */
    i32_t index;
    i32_t eval = 0;
    renderBoard(&board);
    printf("Turn: %s\n", board.color == 1 ? "P1" : "P2");

    /**
     * Game loop
     * Make modifications for human vs human, human vs ai, ai vs ai here
    */
    while (!(isBoardPlayerOneEmpty(&board) || isBoardPlayerTwoEmpty(&board))) {
        // Check if human or ai turn
        if (board.color == 1) {
            printf("Enter move: ");
            // Catch non integer input
            if (scanf("%d", &index) != 1) {
                printf("Invalid move\n");
                // Clear input buffer
                while (getchar() != '\n');
                continue;
            }
            // Check bounds
            if (index < 1 || index > 6 || board.cells[index - 1] == 0) {
                printf("Invalid move\n");
                continue;
            }
            // Translate to 0 start index
            index -= 1;
        } else {
            // Call ai
            negamaxRoot(&board, &index, &eval, aiDepth);
            printf("AI move: %d\n", 13 - index);
        }

        // Perform move
        makeMoveManual(&board, index);
        renderBoard(&board);
        printf("Turn: %s; Evaluation: %d;\n", board.color == 1 ? "P1" : "P2", eval * board.color);
    }

    return 0;
}