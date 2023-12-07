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

// Construct board at pointer
void newBoard(uint8_t *cells, bool *turn) {
    // Assign start values
    for (int32_t i = 0; i < 14; i++) {
        cells[i] = START_VALUE;
    }
    cells[SCORE_P1] = 0;
    cells[SCORE_P2] = 0;

    // Assign starting player
    (*turn) = true;
}

void copyBoard(const uint8_t *cells, uint8_t *cellsCopy) {
    // Copy values
    memcpy(cellsCopy, cells, 14 * sizeof(uint8_t));
}

// Just dont question it
void renderBoard(const uint8_t *cells) {
    printf("    ┌─");
    for (int32_t i = 0; i < 5; ++i) {
        printf("──┬─");
    }
    printf("──┐\n");

    printf("┌───┤");
    for (int32_t i = 12; i > 7; --i) {
        printf("%2d │", cells[i]);
    }
    printf("%2d ├───┐\n", cells[7]);

    printf("│%2d ├─", cells[SCORE_P2]);
    for (int32_t i = 0; i < 5; ++i) {
        printf("──┼─");
    }
    printf("──┤%2d │\n", cells[SCORE_P1]);

    printf("└───┤");
    for (int32_t i = 0; i < 5; ++i) {
        printf("%2d │", cells[i]);
    }
    printf("%2d ├───┘\n", cells[5]);

    printf("    └─");
    for (int32_t i = 0; i < 5; ++i) {
        printf("──┴─");
    }
    printf("──┘\n");
}

// Returns relative evaluation
int32_t getBoardEvaluation(const uint8_t *cells) {
    return cells[SCORE_P2] - cells[SCORE_P1];
}

// Check if player ones side is empty
bool isBoardPlayerOneEmpty(const uint8_t *cells) {
    return !(*(int64_t*)cells & 0x0000FFFFFFFFFFFF);
}

// Check if player twos side is empty
bool isBoardPlayerTwoEmpty(const uint8_t *cells) {
    return !(*(int64_t*)(cells + 7) & 0x0000FFFFFFFFFFFF);
}

void processBoardTerminal(uint8_t *cells) {
    // Check for finish
    if (isBoardPlayerOneEmpty(cells)) {
        for (int32_t i = 7; i < 13; i++) {
            cells[SCORE_P2] += cells[i];
            cells[i] = 0;
        }
    } else if (isBoardPlayerTwoEmpty(cells)) {
        for (int32_t i = 0; i < 6; i++) {
            cells[SCORE_P1] += cells[i];
            cells[i] = 0;
        }
    }
}

void makeMoveOnBoard(uint8_t *cells, bool *turn, int32_t index) {
    // Propagate stones
    int32_t stones = cells[index];
    cells[index] = 0;

    int i = index + 1;
    for(; i < index + stones + 1; i++) {
        cells[i % 14]++;
    }
    index = (i - 1) % 14;

    // Check if last stone was placed on score field
    // If yes return without inverting turn
    if ((index == SCORE_P1 && *turn) || (index == SCORE_P2 && !*turn)) {
        processBoardTerminal(cells);
        return;
    }

    // Check if last stone was placed on empty field
    // If yes "Steal" stones
    if (cells[index] == 1) {
        int32_t targetIndex = 12 - index;
        // Make sure there even are stones on the other side
        int32_t targetValue = cells[targetIndex];
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

    // Check if this resulted in terminality
    processBoardTerminal(cells);

    // Return no extra move
    *turn = !*turn;
}

// Min
int32_t min(int32_t a, int32_t b) {
    return (a < b) ? a : b;
}

// Max
int32_t max(int32_t a, int32_t b) {
    return (a > b) ? a : b;
}

/* MINIMAX */
int32_t minimax(uint8_t *cells, bool turn, int32_t depth, int32_t alpha, int32_t beta) {
    // Check if we are at terminal state
    // If yes add up all remaining stones and return
    if (isBoardPlayerOneEmpty(cells) || isBoardPlayerTwoEmpty(cells)) {
        return getBoardEvaluation(cells);
    }

    if (depth == 0) {
        return getBoardEvaluation(cells);
    }

    // Needed for remaining code
    uint8_t cellsCopy[14];
    bool newTurn;
    int32_t reference;
    int32_t i;

    // Seperation by whos turn it is
    if (turn) {
        reference = INT32_MAX;
        for (i = 0; i < 6; i++) {
            if (cells[i] == 0) {
                continue;
            }
            newTurn = turn;
            copyBoard(cells, cellsCopy);
            makeMoveOnBoard(cellsCopy, &newTurn, i);
            reference = min(reference, minimax(cellsCopy, newTurn, depth -1, alpha, beta));
            if (reference <= alpha) {
                break;
            }
            beta = min(reference, beta);
        }
    } else {
        reference = INT32_MIN;
        for (i = 7; i < 13; i++) {
            if (cells[i] == 0) {
                continue;
            }
            newTurn = turn;
            copyBoard(cells, cellsCopy);
            makeMoveOnBoard(cellsCopy, &newTurn, i);
            reference = max(reference, minimax(cellsCopy, newTurn, depth - 1, alpha, beta));
            if (reference >= beta) {
                break;
            }
            alpha = max(reference, alpha);
        }
    }

    // Return board evaluation
    return reference;
}

void minimaxRoot(uint8_t *cells, bool turn, int32_t depth, int32_t* move, int32_t* evaluation) {
    int32_t bestValue;
    int32_t bestIndex;

    if (turn) {
        bestValue = INT32_MAX;
        for (int32_t i = 0; i < 6; i++) {
            if (cells[i] == 0) {
                continue;
            }
            uint8_t cellsCopy[14];
            bool newTurn = turn;
            copyBoard(cells, cellsCopy);
            makeMoveOnBoard(cellsCopy, &newTurn, i);
            int32_t value = minimax(cellsCopy, newTurn, depth - 1, INT32_MIN, INT32_MAX);
            if (value < bestValue) {
                bestIndex = i;
                bestValue = value;
            }
        }
    } else {
        bestValue = INT32_MIN;
        for (int32_t i = 7; i < 13; i++) {
            if (cells[i] == 0) {
                continue;
            }
            uint8_t cellsCopy[14];
            bool newTurn = turn;
            copyBoard(cells, cellsCopy);
            makeMoveOnBoard(cellsCopy, &newTurn, i);
            int32_t value = minimax(cellsCopy, newTurn, depth - 1, INT32_MIN, INT32_MAX);
            if (value > bestValue) {
                bestIndex = i;
                bestValue = value;
            }
        }
    }

    *move = bestIndex;
    *evaluation = bestValue;
}

void renderBoardWithNextMove(const uint8_t *cells, const bool turn) {
    renderBoard(cells);
    printf("Move: %d\n", turn);
}

int32_t main(int32_t argc, char const *argv[])
{
    uint8_t cells[14];
    bool turn = true;
    newBoard(cells, &turn);
    renderBoardWithNextMove(cells, turn);

    while (!(isBoardPlayerOneEmpty(cells) || isBoardPlayerTwoEmpty(cells))) {
        int32_t index;
        int32_t eval;
        minimaxRoot(cells, turn, 16, &index, &eval);
        makeMoveOnBoard(cells, &turn, index);
        renderBoardWithNextMove(cells, turn);
        printf("Eval: %d\n", -eval);
    }

    return 0;
}