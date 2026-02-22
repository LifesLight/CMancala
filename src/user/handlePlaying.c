/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "user/handlePlaying.h"


void renderPlayHelp() {
    renderOutput("Commands:", PLAY_PREFIX);
    renderOutput("  move [idx]                       : Move the cell at index", PLAY_PREFIX);
    renderOutput("  menu                             : Return to the game menu", PLAY_PREFIX);
    renderOutput("  help                             : Print this help", PLAY_PREFIX);
    renderOutput("  quit                             : Quit the application", PLAY_PREFIX);
}

void getMoveAi(Context* context) {
    aspirationRoot(context, &context->config.solverConfig);
}

void getMoveRandom(Context* context) {
    Board* board = context->board;

    const int8_t start = (board->color == 1)  ? HBOUND_P1 : HBOUND_P2;
    const int8_t end = (board->color == 1)    ? LBOUND_P1 : LBOUND_P2;

    // First, count the non-empty cells and store their indices
    int nonEmptyCells[6];
    int count = 0;
    for (int8_t i = start; i >= end; i--) {
        if (board->cells[i] != 0) {
            nonEmptyCells[count++] = i;
        }
    }

    // Only proceed if there is at least one non-empty cell
    if (count > 0) {
        int randomIndex = rand() % count;
        context->metadata.lastMove = nonEmptyCells[randomIndex];
    } else {
        renderOutput("Random agent could not find valid move", PLAY_PREFIX);
        quitGame();
    }

    return;
}

void tellContextNoComputation(Context* context) {
    context->metadata.lastEvaluation = INT32_MAX;
    context->metadata.lastSolved = false;
}

void getMoveHuman(bool* requestMenu, Context* context) {
    while (true) {
        char* input = malloc(256);
        getInput(input, PLAY_PREFIX);

        // Check for request for menu
        if (strcmp(input, "menu") == 0) {
            *requestMenu = true;
            return;
        }

        // If empty just continue
        if (strcmp(input, "") == 0) {
            continue;
        }

        // Check for help
        if (strcmp(input, "help") == 0) {
            renderPlayHelp();
            continue;
        }

        // Try parsing just number to move
        if (atoi(input) != 0) {
            int idx = atoi(input);

            if (idx < 1 || idx >= 7) {
                renderOutput("Invalid index", CHEAT_PREFIX);
                continue;
            }

            idx = idx - 1;

            if (context->board->color == -1) {
                idx = 5 - idx + 7;
            }

            Board* board = context->board;
            if (board->cells[idx] == 0) {
                renderOutput("Cell is empty", CHEAT_PREFIX);
                continue;;
            }

            context->metadata.lastMove = idx;

            break;
        }

        // Check for move
        if (strncmp(input, "move", 4) == 0) {
            int idx = atoi(input + 5);
            if (idx < 1 || idx > 6) {
                renderOutput("Invalid index", CHEAT_PREFIX);
                continue;
            }

            idx = idx - 1;

            Board* board = context->board;
            if (board->cells[idx] == 0) {
                renderOutput("Cell is empty", CHEAT_PREFIX);
                continue;
            }

            context->metadata.lastMove = idx;
            return;
        }

        char message[256];
        snprintf(message, sizeof(message), "Unknown command: \"%s\". Type \"help\" to get all current commands", input);
        renderOutput(message, PLAY_PREFIX);
    }

    return;
}

void stepGame(bool* requestedMenu, Context* context) {
    renderBoard(context->board, PLAY_PREFIX, &context->config.gameSettings);

    // Check turn
    if (context->board->color == 1) {
        switch (context->config.gameSettings.player1) {
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
        switch (context->config.gameSettings.player2) {
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

    int move = context->metadata.lastMove;

    if (move == -1) {
        renderOutput("No move", PLAY_PREFIX);
        *requestedMenu = true;
        return;
    }

    char message[256];
    snprintf(message, sizeof(message), "Move: %d", move > 5 ? 13 - move : move + 1);
    renderOutput(message, PLAY_PREFIX);

    *context->lastBoard = *context->board;
    makeMoveManual(context->board, move);
    return;
}
