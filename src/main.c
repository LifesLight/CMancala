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
    // Initialize "Main" board with default values
    Board board;
    configBoard(&board, 4);

    // Default AI time limit in seconds
    double timeLimit = 5.0;

    // Default depth for negamaxRootDepth (0 means time-based search is used)
    int depth = 0;

    // Starting color
    int startColor = 1;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--stones") == 0 && i + 1 < argc) {
            configBoard(&board, atoi(argv[++i]));
        } else if (strcmp(argv[i], "--rstones") == 0 && i + 1 < argc) {
            configBoardRand(&board, atoi(argv[++i]));
        } else if (strcmp(argv[i], "--time") == 0 && i + 1 < argc) {
            timeLimit = atof(argv[++i]);
        } else if (strcmp(argv[i], "--depth") == 0 && i + 1 < argc) {
            depth = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--ai-start") == 0) {
            startColor = -1;
        } else if (strcmp(argv[i], "--human-start") == 0) {
            startColor = 1;
        }
    }

    // Set starting color
    board.color = startColor;

    // Initial rendering of board
    renderBoard(&board);
    printf("Turn: %s\n", board.color == 1 ? "P1" : "P2");

    // Game loop
    int index;
    int eval = 0;
    int referenceEval = 0;

    while (!(isBoardPlayerOneEmpty(&board) || isBoardPlayerTwoEmpty(&board))) {
        // Check if human or AI turn
        if (board.color == 1) {
            printf("Enter move: ");
            if (scanf("%d", &index) != 1) {
                printf("Invalid move\n");
                // Clear input buffer
                while (getchar() != '\n');
                continue;
            }
            if (index < 1 || index > 6 || board.cells[index - 1] == 0) {
                printf("Invalid move\n");
                continue;
            }
            // Translate to 0 start index
            index -= 1;
        } else {
            // Is depth or time based search used?
            if (depth > 0) {
                negamaxRootDepth(&board, &index, &eval, depth);
            } else {
                negamaxRootTime(&board, &index, &eval, timeLimit);
            }
            referenceEval = eval * board.color;
            printf("AI move: %d\n", 13 - index);
        }

        makeMoveManual(&board, index);
        renderBoard(&board);
        printf("Turn: %s; Evaluation: %d;\n", board.color == 1 ? "P1" : "P2", referenceEval);
    }

    return 0;
}
