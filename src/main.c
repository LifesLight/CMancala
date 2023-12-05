#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Board is constructed like this:
 * Score cells are 6 and 13
 * Indecies P1: 0 - 5
 * Indecies P2: 7 - 12
 */

#define SCORE_P1 6
#define SCORE_P2 13
#define START_VALUE 4

struct Board
{
    uint8_t cells[14];
    // False: Player1 turn
    bool turn;
};

typedef struct Board Board;

// Construct board at pointer
void newBoard(Board* b) {
    // Assign start values
    for (int i = 0; i < 14; i++) {
        b->cells[i] = START_VALUE;
    }
    b->cells[SCORE_P1] = 0;
    b->cells[SCORE_P2] = 0;

    // Assign starting player
    b->turn = true;
}

void copyBoard(const Board* b, Board* bCopy) {
    // Copy values
    memcpy(bCopy, b, sizeof(Board));
}

// Just dont question it
void renderBoard(const Board *b) {
    printf("    ┌─");
    for (int i = 0; i < 5; ++i) {
        printf("──┬─");
    }
    printf("──┐\n");

    printf("┌───┤");
    for (int i = 12; i > 7; --i) {
        printf("%2d │", b->cells[i]);
    }
    printf("%2d ├───┐\n", b->cells[7]);

    printf("│%2d ├─", b->cells[SCORE_P2]);
    for (int i = 0; i < 5; ++i) {
        printf("──┼─");
    }
    printf("──┤%2d │\n", b->cells[SCORE_P1]);

    printf("└───┤");
    for (int i = 0; i < 5; ++i) {
        printf("%2d │", b->cells[i]);
    }
    printf("%2d ├───┘\n", b->cells[5]);

    printf("    └─");
    for (int i = 0; i < 5; ++i) {
        printf("──┴─");
    }
    printf("──┘\n");
}

// Returns relative evaluation
int getBoardEvaluation(const Board* b) {
    return b->cells[SCORE_P2] - b->cells[SCORE_P1];
}

// Check if player ones side is empty
bool isBoardPlayerOneEmpty(const Board* b) {
    return !(*(int64_t*)b->cells & 0x0000FFFFFFFFFFFF);
}

// Check if player twos side is empty
bool isBoardPlayerTwoEmpty(const Board* b) {
    return !(*(int64_t*)(b->cells + 7) & 0x0000FFFFFFFFFFFF);
}

void processBoardTerminal(Board* b) {
    // Check for finish
    if (isBoardPlayerOneEmpty(b)) {
        for (int i = 7; i < 13; i++) {
            b->cells[SCORE_P2] += b->cells[i];
            b->cells[i] = 0;
        }
    } else if (isBoardPlayerTwoEmpty(b)) {
        for (int i = 0; i < 6; i++) {
            b->cells[SCORE_P1] += b->cells[i];
            b->cells[i] = 0;
        }
    }
}

void makeMoveOnBoard(Board* b, int index) {
    // Propagate stones
    int stones = b->cells[index];
    b->cells[index] = 0;
    while (stones > 0) {
        index++;
        index %= 14;
        b->cells[index]++;
        stones--;
    }

    // Check if last stone was placed on score field
    // If yes return without inverting turn
    if ((index == SCORE_P1 && b->turn) || (index == SCORE_P2 && !b->turn)) {
        processBoardTerminal(b);
        return;
    }

    // Check if last stone was placed on empty field
    // If yes "Steal" stones
    if (b->cells[index] == 1) {
        int targetIndex = 12 - index;
        // Make sure there even are stones on the other side
        int targetValue = b->cells[targetIndex];
        if (targetValue > 0) {
            // If player 2 made move
            if (!b->turn && index > SCORE_P1 && targetValue > 0) {
                b->cells[SCORE_P2] += targetValue + 1;
                b->cells[targetIndex] = 0;
                b->cells[index] = 0;
            // Player 1 made move
            } else if (b->turn && index < SCORE_P1) {
                b->cells[SCORE_P1] += b->cells[targetIndex] + 1;
                b->cells[targetIndex] = 0;
                b->cells[index] = 0;
            }
        }
    }

    // Check if this resulted in terminality
    processBoardTerminal(b);

    // Return no extra move
    b->turn = !b->turn;
}



// Min
int min(int a, int b) {
    return (a < b) ? a : b;
}

// Max
int max(int a, int b) {
    return (a > b) ? a : b;
}

/* MINIMAX */
int minimax(Board* b, int alpha, int beta) {
    // Check if we are at terminal state
    // If yes add up all remaining stones and return
    if (isBoardPlayerOneEmpty(b) || isBoardPlayerTwoEmpty(b)) {
        return getBoardEvaluation(b);
    }

    // Basic MinMax code
    int reference;
    int i;
    if (b->turn) {
        reference = INT32_MAX;
        for (i = 0; i < 6; i++) {
            if (b->cells[i] == 0) {
                continue;
            }
            Board bCopy;
            copyBoard(b, &bCopy);
            makeMoveOnBoard(&bCopy, i);
            reference = min(reference, minimax(&bCopy, alpha, beta));
            if (reference <= alpha) {
                break;
            }
            beta = min(reference, beta);
        }
    } else {
        reference = INT32_MIN;
        for (i = 7; i < 13; i++) {
            if (b->cells[i] == 0) {
                continue;
            }
            Board bCopy;
            copyBoard(b, &bCopy);
            makeMoveOnBoard(&bCopy, i);
            reference = max(reference, minimax(&bCopy, alpha, beta));
            if (reference >= beta) {
                break;
            }
            alpha = max(reference, alpha);
        }
    }

    // Return board evaluation
    return reference;
}

void minimaxRoot(Board* b, int* move, int* evaluation) {
    int bestValue;
    int bestIndex;

    if (b->turn) {
        bestValue = INT32_MAX;
        for (int i = 0; i < 6; i++) {
            if (b->cells[i] == 0) {
                continue;
            }
            Board bCopy;
            copyBoard(b, &bCopy);
            makeMoveOnBoard(&bCopy, i);
            int value = minimax(&bCopy, INT32_MIN, INT32_MAX);
            if (value < bestValue) {
                bestIndex = i;
                bestValue = value;
            }
        }
    } else {
        bestValue = INT32_MIN;
        for (int i = 7; i < 13; i++) {
            if (b->cells[i] == 0) {
                continue;
            }
            Board bCopy;
            copyBoard(b, &bCopy);
            makeMoveOnBoard(&bCopy, i);
            int value = minimax(&bCopy, INT32_MIN, INT32_MAX);
            if (value > bestValue) {
                bestIndex = i;
                bestValue = value;
            }
        }
    }

    *move = bestIndex;
    *evaluation = bestValue;
}

void renderBoardWithNextMove(const Board* b) {
    renderBoard(b);
    printf("Move: %d\n", b->turn);
}

int main(int argc, char const *argv[])
{
    Board b;
    newBoard(&b);
    renderBoardWithNextMove(&b);

    while (!(isBoardPlayerOneEmpty(&b) || isBoardPlayerTwoEmpty(&b))) {
        int index;
        int eval;
        minimaxRoot(&b, &index, &eval);
        makeMoveOnBoard(&b, index);
        renderBoardWithNextMove(&b);
        printf("Eval: %d\n", eval);
    }

    return 0;
}