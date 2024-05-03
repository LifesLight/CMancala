/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "interface.h"

void printConfig(Config* config) {
    char* message;
    renderOutput("Current configuration:", CONFIG_PREFIX);
    asprintf(&message, "  Stones: %d", config->stones);
    renderOutput(message, CONFIG_PREFIX);
    asprintf(&message, "  Distribution: %s", config->distribution == UNIFORM ? "uniform" : "random");
    if (config->distribution == RANDOM) {
        asprintf(&message, "%s (seed: %d)", message, config->seed);
    }
    renderOutput(message, CONFIG_PREFIX);
    asprintf(&message, "  Time: %.2f", config->timeLimit);
    renderOutput(message, CONFIG_PREFIX);
    asprintf(&message, "  Depth: %d", config->depth);
    renderOutput(message, CONFIG_PREFIX);
    asprintf(&message, "  Starting: %s", config->startColor == 1 ? "human" : "ai");
    renderOutput(message, CONFIG_PREFIX);
    asprintf(&message, "  Autoplay: %s", config->autoplay ? "true" : "false");
    renderOutput(message, CONFIG_PREFIX);
}

void getInput(char* input, const char* prefix) {
    printf("%s%s", prefix ,INPUT_PREFIX);
    fgets(input, 256, stdin);

    // Remove newline
    input[strcspn(input, "\n")] = 0;

    // Remove trailing spaces
    while (input[strlen(input) - 1] == ' ') {
        input[strlen(input) - 1] = 0;
    }

    // Remove leading spaces
    while (input[0] == ' ') {
        input++;
    }
}

void initializeBoardFromConfig(Board* board, Config* config) {
    // Initialize board
    if (config->distribution == UNIFORM) {
        configBoard(board, config->stones);
    } else {
        configBoardRand(board, config->stones, config->seed);
    }
    board->color = config->startColor;
}

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

void handleConfigInput(bool* requestedQuit, Config* config) {
    // Wait for user input
    char* input = malloc(256);
    getInput(input, CONFIG_PREFIX);

    // Parse input
    // If just newline (empty) continue
    if (strlen(input) == 0) {
        free(input);
        return;
    }

    // Check for quit
    if (strcmp(input, "quit") == 0 || strcmp(input, "q") == 0) {
        *requestedQuit = true;
        free(input);
        return;
    }

    // Check for help
    if (strcmp(input, "help") == 0) {
        renderConfigHelp();
        free(input);
        return;
    }

    // Check for start
    if (strcmp(input, "start") == 0) {
        startGameHandling(config);
        free(input);
        return;
    }

    // Check for display
    if (strcmp(input, "display") == 0) {
        printConfig(config);
        free(input);
        return;
    }

    // Check for stones
    if (strncmp(input, "stones ", 7) == 0) {
        // Check if valid number
        int stones = atoi(input + 7);

        if (stones <= 0) {
            renderOutput("Invalid number of stones", CONFIG_PREFIX);
            free(input);
            return;
        }

        // Update config
        config->stones = stones;

        char* message;
        asprintf(&message, "Updated stones to %d", stones);
        renderOutput(message, CONFIG_PREFIX);
        free(input);
        return;
    }

    // Check for seed
    if (strncmp(input, "seed ", 5) == 0) {
        // Check if valid number
        int seed = atoi(input + 5);

        // Update config
        config->seed = seed;

        char* message;
        asprintf(&message, "Updated seed to %d", seed);
        renderOutput(message, CONFIG_PREFIX);
        free(input);
        return;
    }

    // Check for time
    if (strncmp(input, "time ", 5) == 0) {
        // Check if valid number
        double time = atof(input + 5);

        if (time < 0) {
            renderOutput("Invalid time limit", CONFIG_PREFIX);
            free(input);
            return;
        }

        // Update config
        config->timeLimit = time;

        char* message;
        asprintf(&message, "Updated time limit to %.2f", time);
        renderOutput(message, CONFIG_PREFIX);
        free(input);
        return;
    }

    // Check for depth
    if (strncmp(input, "depth ", 6) == 0) {
        // Check if valid number
        int depth = atoi(input + 6);

        if (depth < 0) {
            renderOutput("Invalid depth limit", CONFIG_PREFIX);
            free(input);
            return;
        }

        // Update config
        config->depth = depth;

        char* message;
        asprintf(&message, "Updated depth limit to %d", depth);
        renderOutput(message, CONFIG_PREFIX);
        free(input);
        return;
    }

    // Check for autoplay
    if (strncmp(input, "autoplay ", 9) == 0) {
        // Save original autoplay
        bool originalAutoplay = config->autoplay;

        // Check if valid autoplay
        if (strcmp(input + 9, "true") == 0) {
            config->autoplay = true;
            if (originalAutoplay) {
                renderOutput("Autoplay already enabled", CONFIG_PREFIX);
                free(input);
                return;
            }

            renderOutput("Enabled autoplay", CONFIG_PREFIX);
            free(input);
            return;
        } else if (strcmp(input + 9, "false") == 0) {
            config->autoplay = false;
            if (!originalAutoplay) {
                renderOutput("Autoplay already disabled", CONFIG_PREFIX);
                free(input);
                return;
            }

            renderOutput("Disabled autoplay", CONFIG_PREFIX);
            free(input);
            return;
        } else {
            char* message;
            asprintf(&message, "Invalid autoplay \"%s\"", input + 9);
            renderOutput(message, CONFIG_PREFIX);
            free(input);
            return;
        }
    }

    // Check for starting
    if (strncmp(input, "starting ", 9) == 0) {
        // Save original starting player
        int originalStartColor = config->startColor;

        // Check if valid starting player
        if (strcmp(input + 9, "human") == 0) {
            config->startColor = 1;
            if (originalStartColor == 1) {
                renderOutput("Starting player already set to human", CONFIG_PREFIX);
                free(input);
                return;
            }

            renderOutput("Updated starting player to human", CONFIG_PREFIX);
            free(input);
            return;
        } else if (strcmp(input + 9, "ai") == 0) {
            config->startColor = 0;
            if (originalStartColor == 0) {
                renderOutput("Starting player already set to ai", CONFIG_PREFIX);
                free(input);
                return;
            }

            renderOutput("Updated starting player to ai", CONFIG_PREFIX);
            free(input);
            return;
        } else {
            char* message;
            asprintf(&message, "Invalid starting player \"%s\"", input + 9);
            renderOutput(message, CONFIG_PREFIX);
            free(input);
            return;
        }
    }

    // Check for distribution
    if (strncmp(input, "distribution ", 13) == 0) {
        // Save original distribution
        enum Distribution originalDistribution = config->distribution;

        // Check if valid distribution
        if (strcmp(input + 13, "uniform") == 0) {
            config->distribution = UNIFORM;
            if (originalDistribution == UNIFORM) {
                renderOutput("Distribution already set to uniform", CONFIG_PREFIX);
                free(input);
                return;
            }

            renderOutput("Updated distribution to uniform", CONFIG_PREFIX);
            free(input);
            return;
        } else if (strcmp(input + 13, "random") == 0) {
            config->distribution = RANDOM;
            if (originalDistribution == RANDOM) {
                renderOutput("Distribution already set to random", CONFIG_PREFIX);
                free(input);
                return;
            }

            renderOutput("Updated distribution to random", CONFIG_PREFIX);
            free(input);
            return;
        } else {
            char* message;
            asprintf(&message, "Invalid distribution \"%s\"", input + 13);
            renderOutput(message, CONFIG_PREFIX);
            free(input);
            return;
        }
    }

    // Unknown command
    char* message;
    asprintf(&message, "Unknown command \"%s\"", input);
    renderOutput(message, CONFIG_PREFIX);
    free(input);
}

void startInterface() {
    // Print welcome message
    renderWelcome();

    // Global config struct
    Config config = {
        .stones = 4,
        .distribution = UNIFORM,
        .seed = time(NULL),
        .timeLimit = 5.0,
        .depth = 0,
        .startColor = 1,
        .autoplay = true
    };

    // Global loop
    bool requestedQuit = false;
    while (!requestedQuit) {;
        handleConfigInput(&requestedQuit, &config);
    }
}
