/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "handleGame.h"

void renderCheatHelp() {
    renderOutput("Commands:", CHEAT_PREFIX);
    renderOutput("  step                             : Step to the next turn", CHEAT_PREFIX);
    renderOutput("  undo                             : Undo the last move", CHEAT_PREFIX);
    renderOutput("  switch                           : Switch the next player", CHEAT_PREFIX);
    renderOutput("  edit [player] [idx] [value]      : Edit cell value", CHEAT_PREFIX);
    renderOutput("  render                           : Render the current board", CHEAT_PREFIX);
    renderOutput("  last                             : Fetch the last moves metadata", CHEAT_PREFIX);
    renderOutput("  autoplay [true|false]            : If enabled the game loop will automatically continue", CONFIG_PREFIX);
    renderOutput("  config                           : Return the config menu. Will discard the current game", CHEAT_PREFIX);
    renderOutput("  help                             : Print this help", CHEAT_PREFIX);
    renderOutput("  quit                             : Quit the application", CHEAT_PREFIX);
}

void handleGameInput(bool* requestedConfig, bool* requestContinue, Context* context) {
    // TODO:
    // Hash gamestate
    // Load hashed gamestate -> maybe in config?
    // Reevaluate Evaluate gamestate

    char* input = malloc(256);
    getInput(input, CHEAT_PREFIX);


    // Check for request to step
    if (strcmp(input, "step") == 0) {
        *requestContinue = true;
        free(input);
        return;
    }

    // Check for request to undo
    if (strcmp(input, "undo") == 0) {
        if (context->lastMove == -1) {
            renderOutput("No move to undo", CHEAT_PREFIX);
            free(input);
            return;
        }

        copyBoard(context->lastBoard, context->board);
        context->lastMove = -1;
        context->lastEvaluation = INT32_MAX;
        context->lastDepth = 0;
        context->lastSolved = false;
        renderOutput("Undid last move", CHEAT_PREFIX);
        free(input);
        return;
    }

    // Check for request to switch
    if (strcmp(input, "switch") == 0) {
        context->board->color = context->board->color * -1;
        renderOutput("Switched player", CHEAT_PREFIX);
        free(input);
        return;
    }

    // Check for request to edit
    if (strncmp(input, "edit ", 5) == 0) {
        // Check for valid input
        if (strlen(input) < 7) {
            renderOutput("Invalid edit command", CHEAT_PREFIX);
            free(input);
            return;
        }

        // Parse player
        int player = 0;
        if (input[5] == '1') {
            player = 1;
        } else if (input[5] == '2') {
            player = -1;
        } else {
            renderOutput("Invalid player", CHEAT_PREFIX);
            free(input);
            return;
        }

        // Parse idx
        int idx = atoi(input + 7);
        if (idx < 1 || idx > 6) {
            renderOutput("Invalid idx", CHEAT_PREFIX);
            free(input);
            return;
        }

        // Parse value
        int value = atoi(input + 9);
        if (value < 0) {
            renderOutput("Invalid value", CHEAT_PREFIX);
            free(input);
            return;
        }

        // Update cell
        updateCell(context->board, player, idx, value);
        free(input);
        return;
    }

    // If empty just return
    if (strcmp(input, "") == 0) {
        free(input);
        return;
    }

    // Check for render
    if (strcmp(input, "render") == 0) {
        renderBoard(context->board, CHEAT_PREFIX, context->config);
        free(input);
        return;
    }

    // Check for last
    if (strcmp(input, "last") == 0) {
        char* message = malloc(256);
        asprintf(&message, "Metadata:");
        renderOutput(message, CHEAT_PREFIX);

        if (context->lastMove != -1) {
            asprintf(&message, "  Move: %d", context->lastMove > 5 ? 13 - context->lastMove : context->lastMove + 1);
            renderOutput(message, CHEAT_PREFIX);
        }

        if (context->lastEvaluation != INT32_MAX) {
            asprintf(&message, "  Depth: %d", context->lastDepth);
            renderOutput(message, CHEAT_PREFIX);

            asprintf(&message, "  Evaluation: %d", context->lastEvaluation);
            renderOutput(message, CHEAT_PREFIX);

            if (context->lastSolved) {
                asprintf(&message, "  Solved: true");
            } else {
                asprintf(&message, "  Solved: false");
            }
            renderOutput(message, CHEAT_PREFIX);
        }

        free(message);
        free(input);
        return;
    }

    // Check for autoplay
    if (strncmp(input, "autoplay ", 9) == 0) {
        // Save original autoplay
        bool originalAutoplay = context->config->autoplay;

        // Check if valid autoplay
        if (strcmp(input + 9, "true") == 0 || strcmp(input + 9, "1") == 0) {
            context->config->autoplay = true;
            if (originalAutoplay) {
                renderOutput("Autoplay already enabled", CHEAT_PREFIX);
                free(input);
                return;
            }

            renderOutput("Enabled autoplay", CHEAT_PREFIX);
            free(input);
            return;
        } else if (strcmp(input + 9, "false") == 0 || strcmp(input + 9, "0") == 0) {
            context->config->autoplay = false;
            if (!originalAutoplay) {
                renderOutput("Autoplay already disabled", CHEAT_PREFIX);
                free(input);
                return;
            }

            renderOutput("Disabled autoplay", CHEAT_PREFIX);
            free(input);
            return;
        } else {
            char* message = malloc(256);
            asprintf(&message, "Invalid autoplay \"%s\"", input + 9);
            renderOutput(message, CHEAT_PREFIX);
            free(message);
            free(input);
            return;
        }
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
    asprintf(&message, "Unknown command: %s. Type \"help\" to get all current commands", input);
    renderOutput(message, CHEAT_PREFIX);
    free(message);
    free(input);
    return;
}

void startGameHandling(Config* config) {
    // Initialize board
    Board board;
    initializeBoardFromConfig(&board, config);

    // Make game context
    Context context = {
        .board = &board,
        .lastBoard = malloc(sizeof(Board)),
        .config = config,
        .lastEvaluation = INT32_MAX,
        .lastMove = -1,
        .lastDepth = 0,
        .lastSolved = false
    };

    // Start game loop
    bool requestedConfig = false;
    bool requestedContinue = config->autoplay ? true : false;
    bool gameOver = false;

    while(!requestedConfig) {
        while(requestedContinue && !gameOver) {
            bool requestedMenu = false;

            stepGame(&requestedMenu, &context);

            if (!config->autoplay || requestedMenu) {
                requestedContinue = false;
                break;
            }

            if (isBoardPlayerOneEmpty(&board) || isBoardPlayerTwoEmpty(&board)) {
                gameOver = true;

                renderBoard(&board, PLAY_PREFIX, config);

                int score1 = board.cells[SCORE_P1];
                int score2 = board.cells[SCORE_P2];

                char* message = malloc(256);
                if (score1 > score2) {
                    asprintf(&message, "Player 1 wins");
                } else if (score1 < score2) {
                    asprintf(&message, "Player 2 wins");
                } else {
                    asprintf(&message, "Draw");
                }

                asprintf(&message, "%s, score: %d - %d", message, score1, score2);
                renderOutput(message, PLAY_PREFIX);

                break;
            }
        }

        handleGameInput(&requestedConfig, &requestedContinue, &context);
    }
}
