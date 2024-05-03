/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "handleGame.h"

void handleGameInput(bool* requestQuit, bool* requestContinue,Board* board, Config* config) {
    // Set next player
    // Edit cell
    // Hash gamestate
    // Load hashed gamestate
    // Evaluate gamestate
    // Quit game
    // Print help
    // Continue game
    renderOutput("Handeling", GAME_PREFIX);
    char* input = malloc(256);
    getInput(input, GAME_PREFIX);


    // Check for request to continue
    if (strcmp(input, "continue") == 0) {
        *requestContinue = true;
        free(input);
        return;
    }

    // Check for request to quit
    if (strcmp(input, "quit") == 0 || strcmp(input, "q") == 0) {
        *requestQuit = true;
        free(input);
        return;
    }
}

void stepGame(bool* requestedMenu, Board* board, Config* config) {
    renderOutput("Stepping", GAME_PREFIX);

    char* input = malloc(256);
    getInput(input, GAME_PREFIX);

    // Check for request for menu
    if (strcmp(input, "menu") == 0 || strcmp(input, "q") == 0 || strcmp(input, "quit") == 0) {
        *requestedMenu = true;
        free(input);
        return;
    }

    return;
}

void startGameHandling(Config* config) {
    // Initialize board
    Board board;
    initializeBoardFromConfig(&board, config);

    // Start game loop
    bool requestedQuit = false;
    bool requestedContinue = config->autoplay ? true : false;

    while(!requestedQuit) {
        while(requestedContinue) {
            bool requestedMenu = false;
            stepGame(&requestedMenu, &board, config);

            if (!config->autoplay || requestedMenu) {
                requestedContinue = false;
                break;
            }
        }

        handleGameInput(&requestedQuit, &requestedContinue, &board, config);
    }
}
