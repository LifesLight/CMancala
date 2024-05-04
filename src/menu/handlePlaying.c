/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "handlePlaying.h"


void renderPlayHelp() {
    renderOutput("Commands:", PLAY_PREFIX);
    renderOutput("  move <idx>                       : Move to the index cell", PLAY_PREFIX);
    renderOutput("  menu                             : Return to the game menu", PLAY_PREFIX);
    renderOutput("  help                             : Print this help", PLAY_PREFIX);
    renderOutput("  quit                             : Quit the application", PLAY_PREFIX);
}

void computeAiMove(Context* context) {
    Board* board = context->board;
    Config* config = context->config;
    negamaxAspirationRoot(board, &context->lastMove, &context->lastEvaluation, config->depth, config->timeLimit);
}

void stepGame(bool* requestedMenu, Context* context) {
    renderBoard(context->board, PLAY_PREFIX);

    

    // Check turn
    if (context->board->color != 1) {
        computeAiMove(context);
    } else {
        while (true) {
            char* input = malloc(256);
            getInput(input, PLAY_PREFIX);

            // Check for quit
            if (strcmp(input, "quit") == 0 || strcmp(input, "q") == 0) {
                free(input);
                quitGame();
                return;
            }

            // Check for request for menu
            if (strcmp(input, "menu") == 0) {
                *requestedMenu = true;
                free(input);
                return;
            }

            // If empty just continue
            if (strcmp(input, "") == 0) {
                free(input);
                continue;
            }

            // Check for help
            if (strcmp(input, "help") == 0) {
                renderPlayHelp();
                free(input);
                continue;
            }

            // Check for move
            if (strncmp(input, "move", 4) == 0) {
                int idx = atoi(input + 5);
                if (idx < 1 || idx >= 6) {
                    renderOutput("Invalid index", CHEAT_PREFIX);
                    free(input);
                    return;
                }

                idx = idx - 1;

                Board* board = context->board;
                if (board->cells[idx] == 0) {
                    renderOutput("Cell is empty", CHEAT_PREFIX);
                    free(input);
                    return;
                }

                context->lastMove = idx;
                context->lastEvaluation = INT32_MAX;
                free(input);
                break;
            }

            char* message = malloc(256);
            asprintf(&message, "Unknown command: %s", input);
            renderOutput(message, PLAY_PREFIX);
            free(input);
            free(message);
        }
    }

    makeMoveManual(context->board, context->lastMove);
    return;
}
