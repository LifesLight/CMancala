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

// Choose between normal and avalanche mode
// #define AVALANCHE


// Relevant indicies
#define LBOUND_P1 0
#define HBOUND_P1 5

#define LBOUND_P2 7
#define HBOUND_P2 12

#define SCORE_P1 6
#define SCORE_P2 13

#define ASIZE 14


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

// Board struct
typedef struct {
    uint8_t cells[ASIZE];
    int8_t color;
} Board;


// Copies board to target pointer
void copyBoard(const Board *board, Board *target) {
    memcpy(target, board, sizeof(Board));
}

// Construct board at pointer with specified cell values
void newBoardCustomStones(Board *board, int stones) {
    // Bounds checking
    if (stones * 12 > UINT8_MAX) {
        printf("[WARNING]: Reducing %d stones per cell to %d to avoid uint8_t overflow\n", stones, UINT8_MAX / 12);
        stones = UINT8_MAX / 12;
    }

    // Assign start values
    for (int i = 0; i < ASIZE; i++) {
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
void randomizeCells(Board *board, const int stones) {
    // Since we are double assigning per 
    int remainingStones = stones / 2;

    // Check for uint8_t bounds
    if (remainingStones > UINT8_MAX / 2) {
        printf("[WARNING]: Reducing %d total stones to %d to avoid uint8_t overflow\n", remainingStones * 2, UINT8_MAX / 2 * 2);
        remainingStones = UINT8_MAX / 2;
    }

    // Reset board to 0
    memset(board->cells, 0, sizeof(board->cells));

    // Assign mirrored random stones
    srand(time(NULL));
    for (int i = 0; i < remainingStones; i++) {
        int index = rand() % 6;
        board->cells[index] += 1;
        board->cells[index + SCORE_P1 + 1] += 1;
    }
}

void renderBoard(const Board *board) {
    // Print indices
    printf("IDX:  ");
    for (int i = 1; i < LBOUND_P2; i++) {
        printf("%d   ", i);
    }
    printf("\n");

    // Top header
    printf("    %s%s", TL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, ET, HL);
    }
    printf("%s%s%s\n", HL, HL, TR);

    // Upper row
    printf("%s%s%s%s%s", TL, HL, HL, HL, ER);
    for (int i = HBOUND_P2; i > LBOUND_P2; --i) {
        printf("%3d%s", board->cells[i], VL);
    }
    printf("%3d%s%s%s%s%s\n", board->cells[LBOUND_P2], EL, HL, HL, HL, TR);

    // Middle separator
    printf("%s%3d%s%s", VL, board->cells[SCORE_P2], EL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, CR, HL);
    }
    printf("%s%s%s%3d%s\n", HL, HL, ER, board->cells[SCORE_P1], VL);

    // Lower row
    printf("%s%s%s%s%s", BL, HL, HL, HL, ER);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%3d%s", board->cells[i], VL);
    }
    printf("%3d%s%s%s%s%s\n", board->cells[HBOUND_P1], EL, HL, HL, HL, BR);

    // Bottom footer
    printf("    %s%s", BL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, EB, HL);
    }
    printf("%s%s%s\n", HL, HL, BR);
}

// Returns relative evaluation
int getBoardEvaluation(const Board *board) {
    // Calculate score delta
    return board->cells[SCORE_P1] - board->cells[SCORE_P2];
}

// Check if player ones side is empty
bool isBoardPlayerOneEmpty(const Board *board) {
    // Casting the array to a single int64_t,
    // mask out the last 2 indices and if that int64_t is 0 the side is empty
    return !(*(int64_t*)board->cells & 0x0000FFFFFFFFFFFF);
}

// Check if player twos side is empty
bool isBoardPlayerTwoEmpty(const Board *board) {
    // Same logic as ^ just offset the pointer to check the other side
    // Also slightly different mask to check from other side (array bounds)
    return !(*(int64_t*)(board->cells + 5) & 0xFFFFFFFFFFFF0000);
}

// Returns wether the game finished
bool processBoardTerminal(Board *board) {
    // Check for finish
    if (isBoardPlayerOneEmpty(board)) {
        // Add up opposing players stones to non empty players score
        for (int i = LBOUND_P2; i < 13; i++) {
            board->cells[SCORE_P2] += board->cells[i];
            board->cells[i] = 0;
        }
        return true;
    }

    if (isBoardPlayerTwoEmpty(board)) {
        for (int i = 0; i < 6; i++) {
            board->cells[SCORE_P1] += board->cells[i];
            board->cells[i] = 0;
        }
        return true;
    }

    return false;
}

// Performs move on the board, the provided player color is updated accordingly
#ifdef AVALANCHE
// Avalanche mode
void makeMoveOnBoard(Board *board, const uint8_t actionIndex) {
    const bool turn = board->color == 1;

    // Get blocked index for this player
    const uint8_t blockedIndex = turn ? (SCORE_P2 - 1) : (SCORE_P1 - 1);
    uint8_t index = actionIndex;

    do {
        // Propagate stones
        uint8_t stones = board->cells[index];
        board->cells[index] = 0;

        // Propagate stones
        for (uint8_t i = 0; i < stones; i++) {
            // Skip blocked index
            if (index == blockedIndex) {
                index += 2;
            } else {
                // If not blocked, normal increment
                index++;
            }

            // Wrap around bounds
            if (index > 13) {
                index = index - ASIZE;
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
    } while (board->cells[index] > 1);

    // Return no extra move
    board->color = -board->color;
}
#else
// Normal mode
void makeMoveOnBoard(Board *board, const uint8_t actionIndex) {
    // Propagate stones
    const uint8_t stones = board->cells[actionIndex];
    board->cells[actionIndex] = 0;

    const bool turn = board->color == 1;

    // Get blocked index for this player
    const uint8_t blockedIndex = turn ? (SCORE_P2 - 1) : (SCORE_P1 - 1);
    uint8_t index = actionIndex;

    // Propagate stones
    for (uint8_t i = 0; i < stones; i++) {
        // Skip blocked index
        if (index == blockedIndex) {
            index += 2;
        } else {
            // If not blocked, normal increment
            index++;
        }

        // Wrap around bounds
        if (index > 13) {
            index = index - ASIZE;
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
        const uint8_t targetIndex = HBOUND_P2 - index;
        // Make sure there even are stones on the other side
        const uint8_t targetValue = board->cells[targetIndex];

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
#endif

// Make move but also automatically handles terminal state
void makeMoveManual(Board* board, int index) {
    makeMoveOnBoard(board, index);
    processBoardTerminal(board);
}

// Min
int min(const int a, const int b) {
    return (a < b) ? a : b;
}

// Max
int max(const int a, const int b) {
    return (a > b) ? a : b;
}

// Negamax implementation
int negamax(Board *board, int alpha, const int beta, const int depth) {
    // Terminally check
    if (processBoardTerminal(board) || depth == 0) {
        return board->color * getBoardEvaluation(board);
    }

    int reference = INT32_MIN;
    int score;

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    Board boardCopy;

    for (int8_t i = start; i >= end; i--) {
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

// Negamax implementation with best move return
int negamaxWithMove(Board *board, int *bestMove, int alpha, const int beta, const int depth) {
    // Terminally check
    if (processBoardTerminal(board) || depth == 0) {
        *bestMove = -1;
        return board->color * getBoardEvaluation(board);
    }

    int reference = INT32_MIN;
    int score;

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    Board boardCopy;

    for (int8_t i = start; i >= end; i--) {
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

        if (score > reference) {
            reference = score;
            *bestMove = i;
        }

        alpha = max(alpha, reference);

        if (alpha >= beta) {
            break;
        }
    }

    return reference;
}

// Negamax root with depth limit
void negamaxRootDepth(Board *board, int *move, int *evaluation, int depth) {
    // Aspiration window stuff
    const int aspirationWindow = 3;
    const int aspirationStep = 2;
    const int depthStep = 1;

    int alpha = INT32_MIN + 1;
    int beta = INT32_MAX;
    int previousScore = INT32_MIN;

    int currentDepth = 1;
    int localWindow = 0;
    int bestMove;

    int windowMisses = 0;

    do {
        // Call negamax
        // We also store the best move here
        int score = negamaxWithMove(board, &bestMove, alpha, beta, currentDepth);

        // Check if score is within window, if not increase window
        // If we hit bounds we also increase window
        if (score <= alpha || score >= beta) {
            localWindow += aspirationStep;
            windowMisses++;
        } else {
            previousScore = score;
            currentDepth += depthStep;
            localWindow = 0;
        }

        // Update alpha and beta
        alpha = previousScore - aspirationWindow - localWindow;
        beta = previousScore + aspirationWindow + localWindow;

    // Check if we are finished
    } while (currentDepth <= depth);

    // Warn if more then 25% of the time was spent outside the window
    if (windowMisses > currentDepth / 4) {
        printf("[WARNING]: High window misses!\n");
    }

    // Last best move will be the best move
    *move = bestMove;
    *evaluation = previousScore;
}

// Negamax root with time limit
void negamaxRootTime(Board *board, int *move, int *evaluation, double timeInSeconds) {
    // Aspiration window stuff
    const int aspirationWindow = 3;
    const int aspirationStep = 2;
    const int depthStep = 1;
    const int depthLimit = 250;

    int alpha = INT32_MIN + 1;
    int beta = INT32_MAX;
    int previousScore = INT32_MIN;

    int currentDepth = 1;
    int localWindow = 0;
    int bestMove;

    clock_t start = clock();

    int windowMisses = 0;

    do {
        // Call negamax
        // We also store the best move here
        int score = negamaxWithMove(board, &bestMove, alpha, beta, currentDepth);

        // Check if score is within window, if not increase window
        if (score <= alpha || score >= beta) {
            localWindow += aspirationStep;
            windowMisses++;
        } else {
            previousScore = score;
            currentDepth += depthStep;
            localWindow = 0;

            // Check if we are finished
            if (currentDepth > depthLimit) {
                break;
            }

            // Break the loop if the time limit is reached
            // Only on valid window
            clock_t currentTime = clock();
            double elapsedTime = ((double) (currentTime - start)) / CLOCKS_PER_SEC;

            if (elapsedTime >= timeInSeconds) {
                break;
            }
        }

        // Update alpha and beta
        alpha = previousScore - aspirationWindow - localWindow;
        beta = previousScore + aspirationWindow + localWindow;

    // Continue until break condition
    } while (true);

    // Print depth reached
    printf("Depth reached: %d\n", currentDepth - 1);

    // Warn if more then 25% of the time was spent outside the window
    if (windowMisses > currentDepth / 4) {
        printf("[WARNING]: High window misses!\n");
    }

    // Last best move will be the best move
    *move = bestMove;
    *evaluation = previousScore;
}


/**
 * Main function
 * Make modifications to search depth, board layout... here
*/
int main(int argc, char const* argv[]) {
    // "Main" board and turn
    Board board;

    /**
     * "Max" time for AI to think in seconds
     * This is not a hard limit, the AI will finish the current iteration
    */
    const double timeLimit = 5;

    /**
     * Initialize board here
     * Choose from the following functions or write your own init
     * just make sure that total stones are < 256
    */
    newBoardCustomStones(&board, 4);
    //randomizeCells(&board, 60);

     /**
     * Inital rendering of board
    */
    int index;
    int eval = 0;
    int referenceEval = 0;
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
            negamaxRootTime(&board, &index, &eval, timeLimit);
            referenceEval = eval * board.color;
            printf("AI move: %d\n", 13 - index);
        }

        // Perform move
        makeMoveManual(&board, index);
        renderBoard(&board);
        printf("Turn: %s; Evaluation: %d;\n", board.color == 1 ? "P1" : "P2", referenceEval);
    }

    return 0;
}
