/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "handleGame.h"

void renderCheatHelp() {
    renderOutput("Commands:", CHEAT_PREFIX);
    renderOutput("  step                             : Step to the next turn", CHEAT_PREFIX);
    renderOutput("  config                           : Return the config menu", CHEAT_PREFIX);
    renderOutput("  help                             : Print this help", CHEAT_PREFIX);
    renderOutput("  quit                             : Quit the application", CHEAT_PREFIX);
}

void renderPlayHelp() {
    renderOutput("Commands:", PLAY_PREFIX);
    renderOutput("  menu                             : Return to the game menu", PLAY_PREFIX);
    renderOutput("  help                             : Print this help", PLAY_PREFIX);
    renderOutput("  quit                             : Quit the application", PLAY_PREFIX);
}

void handleGameInput(bool* requestedConfig, bool* requestContinue,Board* board, Config* config) {
    // Set next player
    // Edit cell
    // Hash gamestate
    // Load hashed gamestate
    // Evaluate gamestate
    // Quit game
    // Print help
    // Continue game
    char* input = malloc(256);
    getInput(input, CHEAT_PREFIX);


    // Check for request to step
    if (strcmp(input, "step") == 0) {
        *requestContinue = true;
        free(input);
        return;
    }

    // If empty just return
    if (strcmp(input, "") == 0) {
        free(input);
        return;
    }

    // Check for request to quit
    if (strcmp(input, "quit") == 0 || strcmp(input, "q") == 0) {
        free(input);
        quitGame();
        return;
    }

    // Check for request to config
    if (strcmp(input, "config") == 0) {
        *requestedConfig = true;
        free(input);
        return;
    }

    // Check for help
    if (strcmp(input, "help") == 0) {
        renderCheatHelp();
        free(input);
        return;
    }

    char* message = malloc(256);
    asprintf(&message, "Unknown command: %s", input);
    renderOutput(message, CHEAT_PREFIX);
    free(message);
    free(input);
    return;
}

void computeAiMove(Context* context) {
    Board* board = context->board;
    Config* config = context->config;
    negamaxAspirationRoot(board, &context->lastMove, &context->lastEvaluation, config->depth, config->timeLimit);
}

void stepGame(bool* requestedMenu, Context* context) {
    // Check turn
    if (!context->board->color) {
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

            char* message = malloc(256);
            asprintf(&message, "Unknown command: %s", input);
            renderOutput(message, PLAY_PREFIX);
            free(input);
            free(message);
        }
    }

    context->board->color = !context->board->color;

    return;
}

void startGameHandling(Config* config) {
    // Initialize board
    Board board;
    initializeBoardFromConfig(&board, config);

    // Make game context
    Context context = {
        .board = &board,
        .config = config,
        .lastEvaluation = 0,
        .lastMove = -1
    };

    // Start game loop
    bool requestedConfig = false;
    bool requestedContinue = config->autoplay ? true : false;

    while(!requestedConfig) {
        if (requestedContinue) {
            renderBoard(context.board, PLAY_PREFIX);
        }

        while(requestedContinue) {
            bool requestedMenu = false;
            stepGame(&requestedMenu, &context);

            if (!config->autoplay || requestedMenu) {
                requestedContinue = false;
                break;
            }
        }

        handleGameInput(&requestedConfig, &requestedContinue, &board, config);
    }
}
