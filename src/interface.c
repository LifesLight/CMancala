/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "interface.h"

void printConfig(struct Config* config) {
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
}

void getInput(const char* input, const char* prefix) {
    printf("%s%s", prefix ,INPUT_PREFIX);
    fgets(input, 256, stdin);
}

void startGameHandling(struct Config* config) {
    return;
}

void handleConfigInput(bool* requestedQuit, struct Config* config) {
    // Wait for user input
    char* input = malloc(256);
    getInput(input, CONFIG_PREFIX);

    // Parse input
    // If just newline, continue
    if (strcmp(input, "\n") == 0) {
        return;
    }

    // Remove newline
    input[strcspn(input, "\n")] = 0;

    // Check for quit
    if (strcmp(input, "quit") == 0 || strcmp(input, "q") == 0) {
        *requestedQuit = true;
        return;
    }

    // Check for help
    if (strcmp(input, "help") == 0) {
        renderConfigHelp();
        return;
    }

    // Check for start
    if (strcmp(input, "start") == 0) {
        startGameHandling(config);
        return;
    }

    // Check for display
    if (strcmp(input, "display") == 0) {
        printConfig(config);
        return;
    }

    // Check for stones
    if (strncmp(input, "stones ", 7) == 0) {
        // Check if valid number
        int stones = atoi(input + 7);

        if (stones <= 0) {
            renderOutput("Invalid number of stones", CONFIG_PREFIX);
            return;
        }

        // Update config
        config->stones = stones;

        char* message;
        asprintf(&message, "Updated stones to %d", stones);
        renderOutput(message, CONFIG_PREFIX);
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

        char* message;
        asprintf(&message, "Updated time limit to %.2f", time);
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

        char* message;
        asprintf(&message, "Updated depth limit to %d", depth);
        renderOutput(message, CONFIG_PREFIX);
        return;
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
                return;
            }

            renderOutput("Updated starting player to human", CONFIG_PREFIX);
            return;
        } else if (strcmp(input + 9, "ai") == 0) {
            config->startColor = 0;
            if (originalStartColor == 0) {
                renderOutput("Starting player already set to ai", CONFIG_PREFIX);
                return;
            }

            renderOutput("Updated starting player to ai", CONFIG_PREFIX);
            return;
        } else {
            char* message;
            asprintf(&message, "Invalid starting player \"%s\"", input + 9);
            renderOutput(message, CONFIG_PREFIX);
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
                return;
            }

            renderOutput("Updated distribution to uniform", CONFIG_PREFIX);
            return;
        } else if (strcmp(input + 13, "random") == 0) {
            config->distribution = RANDOM;
            if (originalDistribution == RANDOM) {
                renderOutput("Distribution already set to random", CONFIG_PREFIX);
                return;
            }

            renderOutput("Updated distribution to random", CONFIG_PREFIX);
            return;
        } else {
            char* message;
            asprintf(&message, "Invalid distribution \"%s\"", input + 13);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    // Unknown command
    char* message;
    asprintf(&message, "Unknown command \"%s\"", input);
    renderOutput(message, CONFIG_PREFIX);
}

void startInterface() {
    // Print welcome message
    renderWelcome();

    // Global config struct
    struct Config config = {
        .stones = 4,
        .distribution = UNIFORM,
        .seed = time(NULL),
        .timeLimit = 5.0,
        .depth = 0,
        .startColor = 1
    };

    // Global loop
    bool requestedQuit = false;
    while (!requestedQuit) {;
        handleConfigInput(&requestedQuit, &config);
    }
}
