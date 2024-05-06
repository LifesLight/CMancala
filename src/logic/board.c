/**
 * Copyright (c) Alexander Kurtz 2023
 */


#include "board.h"

void setMoveFunction(MoveFunction moveFunction) {
    switch (moveFunction) {
        case CLASSIC_MOVE:
            makeMoveFunction = makeMoveOnBoardClassic;
            break;
        case AVALANCHE_MOVE:
            makeMoveFunction = makeMoveOnBoardAvalanche;
            break;
    }
}

void copyBoard(const Board *board, Board *target) {
    memcpy(target, board, sizeof(Board));
}

void configBoard(Board *board, int stones) {
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

void configBoardRand(Board *board, const int stones) {
    int remainingStones = stones * 6;

    // Reset board to 0
    memset(board->cells, 0, sizeof(board->cells));

    // Assign mirrored random stones
    for (int i = 0; i < remainingStones; i++) {
        int index = rand() % 6;
        board->cells[index] += 1;
        board->cells[index + SCORE_P1 + 1] += 1;
    }
}

int getBoardEvaluation(const Board *board) {
    // Calculate score delta
    return board->cells[SCORE_P1] - board->cells[SCORE_P2];
}

bool isBoardPlayerOneEmpty(const Board *board) {
    // Casting the array to a single int64_t,
    // mask out the last 2 indices and if that int64_t is 0 the side is empty
    return !(*(int64_t*)board->cells & 0x0000FFFFFFFFFFFF);
}

bool isBoardPlayerTwoEmpty(const Board *board) {
    // Same logic as ^ just offset the pointer to check the other side
    // Also slightly different mask to check from other side (array bounds)
    return !(*(int64_t*)(board->cells + 5) & 0xFFFFFFFFFFFF0000);
}

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

// Avalanche mode
void makeMoveOnBoardAvalanche(Board *board, const uint8_t actionIndex) {
    const bool turn = board->color == 1;

    // Get blocked index for this player
    const uint8_t blockedIndex = turn ? (SCORE_P2 - 1) : (SCORE_P1 - 1);
    uint8_t index = actionIndex;
    uint8_t stones;

    do {
        // Propagate stones
        stones = board->cells[index];
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

// Normal mode
void makeMoveOnBoardClassic(Board *board, const uint8_t actionIndex) {
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
                // + 1 because we also take the stone from our cell
                board->cells[SCORE_P2] += targetValue + 1;
                board->cells[targetIndex] = 0;
                board->cells[index] = 0;
            // Player 1 made move
            } else if (turn && index < SCORE_P1) {
                board->cells[SCORE_P1] += targetValue + 1;
                board->cells[targetIndex] = 0;
                board->cells[index] = 0;
            }
        }
    }

    // Return no extra move
    board->color = -board->color;
}

void makeMoveManual(Board* board, int index) {
    makeMoveFunction(board, index);
    processBoardTerminal(board);
}

#ifdef HASHING
__uint128_t hashBoard(const Board *board) {
    __uint128_t packed = 0;

    // Store playing color in the first bit
    packed |= ((__uint128_t)(board->color == 1 ? 1 : 0));

    // Pack each cell's stones as 8 bits each, starting from the 2nd bit position
    int shift = 1;
    for (int i = 0; i < 14; i++) {
        // Use 8 bits for each cell
        packed |= ((__uint128_t)(board->cells[i] & 0xFF)) << shift;
        shift += 8;
    }

    return packed;
}

void loadBoard(Board *board, const __uint128_t packed) {
    // Load color from the first bit
    board->color = (packed & 0x01) ? 1 : -1;

    // Unpack each cell's stones from the packed value, 8 bits each, starting from the 2nd bit
    int shift = 1;
    for (int i = 0; i < 14; i++) {
        // Retrieve 8 bits for each cell
        board->cells[i] = (packed >> shift) & 0xFF;
        shift += 8;
    }
}
#endif
