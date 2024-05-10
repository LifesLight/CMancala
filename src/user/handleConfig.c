/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "user/handleConfig.h"

void renderConfigHelp() {
    renderOutput("Commands:", CONFIG_PREFIX);
    renderOutput("  start                            : Start the game", CONFIG_PREFIX);
    renderOutput("  stones [number > 0]              : Set number of stones per pit", CONFIG_PREFIX);
    renderOutput("  distribution [uniform|random]    : Configure distribution of stones", CONFIG_PREFIX);
    renderOutput("  seed [number]                    : Set seed for random distribution, if 0 device time", CONFIG_PREFIX);
    renderOutput("  mode [classic|avalanche]         : Set game mode", CONFIG_PREFIX);
    renderOutput("  time [number >= 0]               : Set time limit for AI in seconds, if 0 unlimited", CONFIG_PREFIX);
    renderOutput("  depth [number >= 0]              : Set depth limit for AI, if 0 unlimited", CONFIG_PREFIX);
    renderOutput("  starting [1|2]                   : Configure starting player", CONFIG_PREFIX);
    renderOutput("  player [1|2] [human|random|ai]   : Configure player", CONFIG_PREFIX);
    renderOutput("  display                          : Display current configuration", CONFIG_PREFIX);
    renderOutput("  autoplay [true|false]            : If enabled the game loop will automatically continue", CONFIG_PREFIX);
    renderOutput("  help                             : Print this help message", CONFIG_PREFIX);
    renderOutput("  quit                             : Quit the application", CONFIG_PREFIX);
}

void printConfig(Config* config) {
    char message[256];

    renderOutput("Current configuration:", CONFIG_PREFIX);
    snprintf(message, sizeof(message), "  Stones: %d", config->stones);
    renderOutput(message, CONFIG_PREFIX);

    switch (config->distribution) {
        case UNIFORM_DIST:
            snprintf(message, sizeof(message), "  Distribution: uniform");
            break;
        case RANDOM_DIST:
            snprintf(message, sizeof(message), "  Distribution: random");
            break;
    }
    renderOutput(message, CONFIG_PREFIX);

    snprintf(message, sizeof(message), "  Seed: %d", config->seed);
    renderOutput(message, CONFIG_PREFIX);

    switch (config->moveFunction) {
        case CLASSIC_MOVE:
            snprintf(message, sizeof(message), "  Mode: classic");
            break;
        case AVALANCHE_MOVE:
            snprintf(message, sizeof(message), "  Mode: avalanche");
            break;
    }
    renderOutput(message, CONFIG_PREFIX);

    snprintf(message, sizeof(message), "  Time: %g%s", config->timeLimit, config->timeLimit == 0 ? " (unlimited)" : "");
    renderOutput(message, CONFIG_PREFIX);

    snprintf(message, sizeof(message), "  Depth: %d%s", config->depth, config->depth == 0 ? " (unlimited)" : "");
    renderOutput(message, CONFIG_PREFIX);

    snprintf(message, sizeof(message), "  Starting: %d", config->startColor == 1 ? 1 : 2);
    renderOutput(message, CONFIG_PREFIX);

    switch (config->player1) {
        case HUMAN_AGENT:
            snprintf(message, sizeof(message), "  Player 1: human");
            break;
        case RANDOM_AGENT:
            snprintf(message, sizeof(message), "  Player 1: random");
            break;
        case AI_AGENT:
            snprintf(message, sizeof(message), "  Player 1: ai");
            break;
    }
    renderOutput(message, CONFIG_PREFIX);

    switch (config->player2) {
        case HUMAN_AGENT:
            snprintf(message, sizeof(message), "  Player 2: human");
            break;
        case RANDOM_AGENT:
            snprintf(message, sizeof(message), "  Player 2: random");
            break;
        case AI_AGENT:
            snprintf(message, sizeof(message), "  Player 2: ai");
            break;
    }
    renderOutput(message, CONFIG_PREFIX);

    snprintf(message, sizeof(message), "  Autoplay: %s", config->autoplay ? "true" : "false");
    renderOutput(message, CONFIG_PREFIX);
}



void handleConfigInput(bool* requestedStart, Config* config) {
    // Wait for user input
    char input[256];
    getInput(input, CONFIG_PREFIX);

    // Parse input
    // If just newline (empty) continue
    if (strlen(input) == 0) {
        return;
    }

    // Check for help
    if (strcmp(input, "help") == 0) {
        renderConfigHelp();
        return;
    }

    // Check for start
    if (strcmp(input, "start") == 0) {
        *requestedStart = true;
        return;
    }

    // Check for display
    if (strcmp(input, "display") == 0) {
        printConfig(config);
        return;
    }

    // Check for mode
    if (strncmp(input, "mode ", 5) == 0) {
        // Save original mode
        MoveFunction originalMode = config->moveFunction;

        // Check if valid mode
        if (strcmp(input + 5, "classic") == 0) {
            config->moveFunction = CLASSIC_MOVE;
            if (originalMode == CLASSIC_MOVE) {
                renderOutput("Mode already set to classic", CONFIG_PREFIX);
                return;
            }

            renderOutput("Updated mode to classic", CONFIG_PREFIX);
            return;
        } else if (strcmp(input + 5, "avalanche") == 0) {
            config->moveFunction = AVALANCHE_MOVE;
            if (originalMode == AVALANCHE_MOVE) {
                renderOutput("Mode already set to avalanche", CONFIG_PREFIX);
                return;
            }

            renderOutput("Updated mode to avalanche", CONFIG_PREFIX);
            return;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid mode \"%s\"", input + 5);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    // Check for stones
    if (strncmp(input, "stones ", 7) == 0) {
        // Check if valid number
        int stones = atoi(input + 7);

        if (stones <= 0) {
            renderOutput("Invalid number of stones", CONFIG_PREFIX);
            return;
        }

        // Validate bounds
        if (stones * 12 > UINT8_MAX) {
            char message[256];
            snprintf(message, sizeof(message), "Reducing %d stones per cell to %d to avoid uint8_t overflow", stones, UINT8_MAX / 12);
            renderOutput(message, CONFIG_PREFIX);
            stones = UINT8_MAX / 12;
        }

        // Update config
        config->stones = stones;

        char message[256];
        snprintf(message, sizeof(message), "Updated stones to %d", stones);
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    // Check for autoplay
    if (strncmp(input, "autoplay ", 9) == 0) {
        // Save original autoplay
        bool originalAutoplay = config->autoplay;

        // Check if valid autoplay
        if (strcmp(input + 9, "true") == 0 || strcmp(input + 9, "1") == 0) {
            config->autoplay = true;
            if (originalAutoplay) {
                renderOutput("Autoplay already enabled", CONFIG_PREFIX);
                return;
            }

            renderOutput("Enabled autoplay", CONFIG_PREFIX);
            return;
        } else if (strcmp(input + 9, "false") == 0 || strcmp(input + 9, "0") == 0) {
            config->autoplay = false;
            if (!originalAutoplay) {
                renderOutput("Autoplay already disabled", CONFIG_PREFIX);
                return;
            }

            renderOutput("Disabled autoplay", CONFIG_PREFIX);
            return;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid autoplay \"%s\"", input + 9);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    // Check for seed
    if (strncmp(input, "seed ", 5) == 0) {
        // Check if valid number
        int seed = atoi(input + 5);

        // Update config
        config->seed = seed;

        if (seed == 0) {
            renderOutput("Changed to device time based seed", CONFIG_PREFIX);
        }

        if (seed == 0) {
            seed = time(NULL);
            config->seed = seed;
        }

        char message[256];
        snprintf(message, sizeof(message), "Updated seed to %d", seed);
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    // Check for time
    if (strncmp(input, "time ", 5) == 0) {
        // Check if valid number
        double time = atof(input + 5);

        if (time < 0) {
            renderOutput("Invalid time limit", CONFIG_PREFIX);
            return;
        }

        // Update config
        config->timeLimit = time;

        char message[256];
        if (time == 0) {
            snprintf(message, sizeof(message), "Updated time limit to unlimited");
        } else {
            snprintf(message, sizeof(message), "Updated time limit to %g", time);
        }
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    // Check for depth
    if (strncmp(input, "depth ", 6) == 0) {
        // Check if valid number
        int depth = atoi(input + 6);

        if (depth < 0) {
            renderOutput("Invalid depth limit", CONFIG_PREFIX);
            return;
        }

        // Update config
        config->depth = depth;

        char message[256];
        if (depth == 0) {
            snprintf(message, sizeof(message), "Updated depth limit to unlimited");
        } else {
            snprintf(message, sizeof(message), "Updated depth limit to %d", depth);
        }
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    // Check for starting
    if (strncmp(input, "starting ", 9) == 0) {
        // Check if valid starting color
        int starting = atoi(input + 9);

        if (starting != 1 && starting != 2) {
            renderOutput("Invalid starting color", CONFIG_PREFIX);
            return;
        }

        // Update config
        config->startColor = starting == 1 ? 1 : -1;

        char message[256];
        snprintf(message, sizeof(message), "Updated starting player to %d", starting);
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    // Check for player
    if (strncmp(input, "player ", 7) == 0) {
        // Check if valid player
        int player = atoi(input + 7);

        if (player != 1 && player != 2) {
            renderOutput("Invalid player", CONFIG_PREFIX);
            return;
        }

        // Check for agent
        if (strncmp(input + 9, "human", 5) == 0) {
            if (player == 1) {
                config->player1 = HUMAN_AGENT;
            } else {
                config->player2 = HUMAN_AGENT;
            }
            char message[256];
            snprintf(message, sizeof(message), "Updated player %d to human", player);
            renderOutput(message, CONFIG_PREFIX);
            return;
        } else if (strncmp(input + 9, "random", 6) == 0) {
            if (player == 1) {
                config->player1 = RANDOM_AGENT;
            } else {
                config->player2 = RANDOM_AGENT;
            }
            char message[256];
            snprintf(message, sizeof(message), "Updated player %d to random", player);
            renderOutput(message, CONFIG_PREFIX);
            return;
        } else if (strncmp(input + 9, "ai", 2) == 0) {
            if (player == 1) {
                config->player1 = AI_AGENT;
            } else {
                config->player2 = AI_AGENT;
            }
            char message[256];
            snprintf(message, sizeof(message), "Updated player %d to ai", player);
            renderOutput(message, CONFIG_PREFIX);
            return;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid agent \"%s\"", input + 9);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    // Check for distribution
    if (strncmp(input, "distribution ", 13) == 0) {
        // Save original distribution
        Distribution originalDistribution = config->distribution;

        // Check if valid distribution
        if (strcmp(input + 13, "uniform") == 0) {
            config->distribution = UNIFORM_DIST;
            if (originalDistribution == UNIFORM_DIST) {
                renderOutput("Distribution already set to uniform", CONFIG_PREFIX);
                return;
            }

            renderOutput("Updated distribution to uniform", CONFIG_PREFIX);
            return;
        } else if (strcmp(input + 13, "random") == 0) {
            config->distribution = RANDOM_DIST;
            if (originalDistribution == RANDOM_DIST) {
                renderOutput("Distribution already set to random", CONFIG_PREFIX);
                return;
            }

            renderOutput("Updated distribution to random", CONFIG_PREFIX);
            return;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid distribution \"%s\"", input + 13);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    // Unknown command
    char message[256];
    snprintf(message, sizeof(message), "Unknown command \"%s\". Type \"help\" to get all current commands", input);
    renderOutput(message, CONFIG_PREFIX);
}
