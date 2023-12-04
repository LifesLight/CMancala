#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/**
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
    bool turn;
};

typedef struct Board Board;


Board* newBoard() {
    struct Board* b = malloc(sizeof(Board));
    // Check if allocation failed
    if (b == NULL) {
        return NULL;
    }

    // Assign start values
    for (int i = 0; i < 14; i++) {
        b->cells[i] = START_VALUE;
    }
    b->cells[SCORE_P1] = 0;
    b->cells[SCORE_P2] = 0;

    // Assign starting player
    b->turn = false;

    // Return finished board
    return b;
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

// Returns if recieved second move
bool makeMoveOnBoard(Board* b, uint8_t index) {
    // Propagate stones
    uint8_t stones = b->cells[index];
    b->cells[index] = 0;
    while (stones > 0) {
        index++;
        index %= 14;
        b->cells[index]++;
        stones--;
    }

    // Check if last stone was placed on score field
    if ((index == SCORE_P1 && !b->turn) || (index == SCORE_P2 && b->turn)) {
        return true;
    }

    // Check if last stone was placed on empty field
    // If yes "Steal" stones
    if (b->cells[index] == 1) {
        int targetIndex = 12 - index;
        // Make sure there even are stones on the other side
        int targetValue = b->cells[targetIndex];
        if (targetValue > 0) {
            // If player 2 made move
            if (b->turn && index > SCORE_P1 && targetValue > 0) {
                b->cells[SCORE_P2] += targetValue + 1;
            // Player 1 made move
            } else if (!b->turn && index < SCORE_P1) {
                b->cells[SCORE_P1] += b->cells[targetIndex] + 1;
            }
            b->cells[targetIndex] = 0;
            b->cells[index] = 0;
        }
    }

    // Return no extra move
    b->turn = !b->turn;
    return false;
}

// Check if player ones side is empty
bool isBoardPlayerOneEmpty(Board* b) {
    return !(*(int64_t*)b->cells & 0x0000FFFFFFFFFFFF);
}

// Check if player twos side is empty
bool isBoardPlayerTwoEmpty(Board* b) {
    return !(*(int64_t*)(b->cells + 7) & 0x0000FFFFFFFFFFFF);
}

int main(int argc, char const *argv[])
{
    Board* myBoard = newBoard();
    renderBoard(myBoard);
    free(myBoard);
    return 0;
}