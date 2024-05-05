/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "handlePlaying.h"


void renderPlayHelp() {
    renderOutput("Commands:", PLAY_PREFIX);
    renderOutput("  move [idx]                       : Move to the index cell", PLAY_PREFIX);
    renderOutput("  menu                             : Return to the game menu", PLAY_PREFIX);
    renderOutput("  help                             : Print this help", PLAY_PREFIX);
    renderOutput("  quit                             : Quit the application", PLAY_PREFIX);
}

void getMoveAi(Context* context) {
    negamaxAspirationRoot(context);
}

void getMoveRandom(Context* context) {
    Board* board = context->board;
    int offset = board->color == 1 ? 0 : 7;

    int idx = rand() % 6 + offset;
    while (board->cells[idx] == 0) {
        idx = rand() % 6 + offset;
    }

    context->lastMove = idx;
    return;
}

void tellContextNoComputation(Context* context) {
    context->lastEvaluation = INT32_MAX;
    context->lastSolved = false;
}

void getMoveHuman(bool* requestMenu, Context* context) {
    while (true) {
        char* input = malloc(256);
        getInput(input, PLAY_PREFIX);

        // Check for request for menu
        if (strcmp(input, "menu") == 0) {
            *requestMenu = true;
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

        // Try parsing just number to move
        if (atoi(input) != 0) {
            int idx = atoi(input);

            if (idx < 1 || idx >= 7) {
                renderOutput("Invalid index", CHEAT_PREFIX);
                free(input);
                continue;;
            }

            idx = idx - 1;

            if (context->board->color == -1) {
                idx = 5 - idx + 7;
            }

            Board* board = context->board;
            if (board->cells[idx] == 0) {
                renderOutput("Cell is empty", CHEAT_PREFIX);
                free(input);
                continue;;
            }

            context->lastMove = idx;

            free(input);
            break;
        }

        // Check for move
        if (strncmp(input, "move", 4) == 0) {
            int idx = atoi(input + 5);
            if (idx < 1 || idx > 6) {
                renderOutput("Invalid index", CHEAT_PREFIX);
                free(input);
                continue;
            }

            idx = idx - 1;

            Board* board = context->board;
            if (board->cells[idx] == 0) {
                renderOutput("Cell is empty", CHEAT_PREFIX);
                free(input);
                continue;
            }

            context->lastMove = idx;

            free(input);
            return;
        }

        char* message = malloc(256);
        asprintf(&message, "Unknown command: %s. Type \"help\" to get all current commands", input);
        renderOutput(message, PLAY_PREFIX);
        free(input);
        free(message);
    }

    return;
}

void stepGame(bool* requestedMenu, Context* context) {
    renderBoard(context->board, PLAY_PREFIX, context->config);

    // Check turn
    if (context->board->color == 1) {
        switch (context->config->player1) {
            case AI_AGENT:
                getMoveAi(context);
                *requestedMenu = false;
                break;
            case RANDOM_AGENT:
                getMoveRandom(context);
                *requestedMenu = false;
                tellContextNoComputation(context);
                break;
            case HUMAN_AGENT:
                getMoveHuman(requestedMenu, context);
                if (!*requestedMenu) {
                    tellContextNoComputation(context);
                }
                break;
        }
    } else {
        switch (context->config->player2) {
            case AI_AGENT:
                getMoveAi(context);
                *requestedMenu = false;
                break;
            case RANDOM_AGENT:
                getMoveRandom(context);
                tellContextNoComputation(context);
                *requestedMenu = false;
                break;
            case HUMAN_AGENT:
                getMoveHuman(requestedMenu, context);
                if (!*requestedMenu) {
                    tellContextNoComputation(context);
                }
                break;
        }
    }

    if (*requestedMenu) {
        return;
    }

    int move = context->lastMove;

    char* message = malloc(256);
    asprintf(&message, "Move: %d", move > 5 ? 13 - move : move + 1);
    renderOutput(message, PLAY_PREFIX);

    copyBoard(context->board, context->lastBoard);
    makeMoveManual(context->board, context->lastMove);
    return;
}
