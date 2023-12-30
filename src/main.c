/**
 * Copyright (c) Alexander Kurtz 2023
*/

#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "render.h"
#include "algo.h"

/**
 * Main function
 * Make modifications to search depth, board layout... here
*/
int main(int argc, char const* argv[]) {
    // "Main" board
    Board board;

    /**
     * "Max" time for AI to think in seconds
     * This is not a hard limit, the AI will finish the current iteration
    */
    const double timeLimit = 2.5;

    /**
     * Initialize board here
     * Choose from the following functions or write your own init
     * just make sure that total stones are < 256
    */
    configBoard(&board, 4);
    // configBoardRand(&board, 60);

    /**
     * Inital rendering of board
    */
    renderBoard(&board);
    printf("Turn: %s\n", board.color == 1 ? "P1" : "P2");

    /**
     * Game loop
     * Make modifications for human vs human, human vs ai, ai vs ai here
    */
    int index;
    int eval = 0;
    int referenceEval = 0;

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
