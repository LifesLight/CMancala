#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

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
    uint8_t* cells[14];
    bool turn;
};

struct Board* new_board() {
    struct Board* b = malloc(sizeof(struct Board));
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


int main(int argc, char const *argv[])
{
    return 0;
}