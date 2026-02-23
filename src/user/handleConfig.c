/**
 * Copyright (c) Alexander Kurtz 2026
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
    renderOutput("  depth [number >= 0]              : Set depth limit for AI, if 0 solve mode", CONFIG_PREFIX);
    renderOutput("  solver [global|local]            : Set default solver for AI", CONFIG_PREFIX);
    renderOutput("  clip [true|false]                : Set clip on/off, clip only computes if a move is winning or losing", CONFIG_PREFIX);
    renderOutput("  cache [number >= 17]             : Set cache size as power of two. If compression is off number needs to be >= 29", CONFIG_PREFIX);
    renderOutput("  compress [always|never|auto]     : Configure cache compression. Auto selects best mode for cache size.", CONFIG_PREFIX);
    renderOutput("  starting [1|2]                   : Configure starting player", CONFIG_PREFIX);
    renderOutput("  player [1|2] [human|random|ai]   : Configure player", CONFIG_PREFIX);
    renderOutput("  display                          : Display current configuration", CONFIG_PREFIX);
    renderOutput("  progress [true|false]            : Configure progress bar visibility during iterative deepening.", CONFIG_PREFIX);
    renderOutput("  autoplay [true|false]            : If enabled the game loop will automatically continue", CONFIG_PREFIX);
    renderOutput("  help                             : Print this help message", CONFIG_PREFIX);
    renderOutput("  quit                             : Quit the application", CONFIG_PREFIX);
}

void printConfig(Config* config) {
    char message[256];

    renderOutput("Current configuration:", CONFIG_PREFIX);
    snprintf(message, sizeof(message), "  Stones: %d", config->gameSettings.stones);
    renderOutput(message, CONFIG_PREFIX);

    switch (config->gameSettings.distribution) {
        case UNIFORM_DIST:
            snprintf(message, sizeof(message), "  Distribution: uniform");
            break;
        case RANDOM_DIST:
            snprintf(message, sizeof(message), "  Distribution: random");
            break;
    }
    renderOutput(message, CONFIG_PREFIX);

    snprintf(message, sizeof(message), "  Seed: %d", config->gameSettings.seed);
    renderOutput(message, CONFIG_PREFIX);

    switch (getMoveFunction()) {
        case CLASSIC_MOVE:
            snprintf(message, sizeof(message), "  Mode: classic");
            break;
        case AVALANCHE_MOVE:
            snprintf(message, sizeof(message), "  Mode: avalanche");
            break;
    }
    renderOutput(message, CONFIG_PREFIX);

    snprintf(message, sizeof(message), "  Time: %g%s", config->solverConfig.timeLimit, config->solverConfig.timeLimit == 0 ? " (unlimited)" : "");
    renderOutput(message, CONFIG_PREFIX);

    snprintf(message, sizeof(message), "  Depth: %d%s", config->solverConfig.depth, config->solverConfig.depth == 0 ? " (unlimited)" : "");
    renderOutput(message, CONFIG_PREFIX);

    switch (config->solverConfig.solver) {
        case GLOBAL_SOLVER:
            snprintf(message, sizeof(message), "  Solver: global");
            break;
        case LOCAL_SOLVER:
            snprintf(message, sizeof(message), "  Solver: local");
            break;
    }

    if (config->solverConfig.clip) {
        char temp[256];
        snprintf(temp, sizeof(temp), " clipped");
        strcat(message, temp);
    }

    renderOutput(message, CONFIG_PREFIX);

    if (getCacheSize() > 0) {
        snprintf(message, sizeof(message), "  Cache size: %-12"PRIu64"", getCacheSize());
        renderOutput(message, CONFIG_PREFIX);
    }

    const char* compressStr = "unknown";
    switch (config->solverConfig.compressCache) {
        case ALWAYS_COMPRESS: compressStr = "always"; break;
        case NEVER_COMPRESS:  compressStr = "never"; break;
        case AUTO:            compressStr = "auto"; break;
    }
    snprintf(message, sizeof(message), "  Compress: %s", compressStr);
    renderOutput(message, CONFIG_PREFIX);

    snprintf(message, sizeof(message), "  Starting: %d", config->gameSettings.startColor == 1 ? 1 : 2);
    renderOutput(message, CONFIG_PREFIX);

    switch (config->gameSettings.player1) {
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

    switch (config->gameSettings.player2) {
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
    char input[256];
    getInput(input, CONFIG_PREFIX);

    if (strlen(input) == 0) {
        return;
    }

    if (strcmp(input, "help") == 0) {
        renderConfigHelp();
        return;
    }

    if (strcmp(input, "start") == 0) {
        *requestedStart = true;
        return;
    }

    if (strcmp(input, "display") == 0) {
        printConfig(config);
        return;
    }

    if (strncmp(input, "cache ", 6) == 0) {
        int cacheSize = atoi(input + 6);

        if (cacheSize < 0) {
            renderOutput("Invalid cache size", CONFIG_PREFIX);
            return;
        }

        setCacheSize(cacheSize);

        if (cacheSize == 0) {
            renderOutput("Disabled cache", CONFIG_PREFIX);
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Updated cache size to %d", cacheSize);
            renderOutput(message, CONFIG_PREFIX);
        }

        return;
    }

    if (strncmp(input, "compress ", 9) == 0) {
        char* val = input + 9;
        CacheMode currentMode = config->solverConfig.compressCache;
        CacheMode newMode = currentMode;

        // Map inputs to CacheMode
        if (strcmp(val, "true") == 0 || strcmp(val, "1") == 0 || strcmp(val, "always") == 0) {
            newMode = ALWAYS_COMPRESS;
        } 
        else if (strcmp(val, "false") == 0 || strcmp(val, "0") == 0 || strcmp(val, "never") == 0) {
            newMode = NEVER_COMPRESS;
        } 
        else if (strcmp(val, "auto") == 0) {
            newMode = AUTO;
        } 
        else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid compress option \"%.200s\"", val);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }

        // Check for state change
        if (newMode == currentMode) {
            // Provide feedback based on what it is currently
            const char* modeStr = (currentMode == ALWAYS_COMPRESS) ? "always" : 
                                  (currentMode == NEVER_COMPRESS) ? "never" : "auto";
            char message[256];
            snprintf(message, sizeof(message), "Compress already set to %s", modeStr);
            renderOutput(message, CONFIG_PREFIX);
        } else {
            config->solverConfig.compressCache = newMode;
            
            const char* modeStr = (newMode == ALWAYS_COMPRESS) ? "always" : 
                                  (newMode == NEVER_COMPRESS) ? "never" : "auto";
            char message[256];
            snprintf(message, sizeof(message), "Updated compress to %s", modeStr);
            renderOutput(message, CONFIG_PREFIX);
        }
        return;
    }

    if (strncmp(input, "clip ", 5) == 0) {
        bool originalClip = config->solverConfig.clip;

        if (strcmp(input + 5, "true") == 0 || strcmp(input + 5, "1") == 0) {
            config->solverConfig.clip = true;
            if (originalClip) {
                renderOutput("Clip already enabled", CONFIG_PREFIX);
                return;
            }
            renderOutput("Enabled clip", CONFIG_PREFIX);
            return;
        } else if (strcmp(input + 5, "false") == 0 || strcmp(input + 5, "0") == 0) {
            config->solverConfig.clip = false;
            if (!originalClip) {
                renderOutput("Clip already disabled", CONFIG_PREFIX);
                return;
            }
            renderOutput("Disabled clip", CONFIG_PREFIX);
            return;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid clip \"%.200s\"", input + 5);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    if (strncmp(input, "solver ", 7) == 0) {
        Solver originalSolver = config->solverConfig.solver;

        if (strcmp(input + 7, "global") == 0) {
            config->solverConfig.solver = GLOBAL_SOLVER;
            if (originalSolver == GLOBAL_SOLVER) {
                renderOutput("Solver already set to global", CONFIG_PREFIX);
                return;
            }
            renderOutput("Updated solver to global", CONFIG_PREFIX);
            return;
        } else if (strcmp(input + 7, "local") == 0) {
            config->solverConfig.solver = LOCAL_SOLVER;
            if (originalSolver == LOCAL_SOLVER) {
                renderOutput("Solver already set to local", CONFIG_PREFIX);
                return;
            }
            renderOutput("Updated solver to local", CONFIG_PREFIX);
            return;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid solver \"%.200s\"", input + 7);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    if (strncmp(input, "mode ", 5) == 0) {
        MoveFunction originalMode = getMoveFunction();

        if (strcmp(input + 5, "classic") == 0) {
            setMoveFunction(CLASSIC_MOVE);
            if (originalMode == CLASSIC_MOVE) {
                renderOutput("Mode already set to classic", CONFIG_PREFIX);
                return;
            }
            renderOutput("Updated mode to classic", CONFIG_PREFIX);
            return;
        } else if (strcmp(input + 5, "avalanche") == 0) {
            setMoveFunction(AVALANCHE_MOVE);
            if (originalMode == AVALANCHE_MOVE) {
                renderOutput("Mode already set to avalanche", CONFIG_PREFIX);
                return;
            }
            renderOutput("Updated mode to avalanche", CONFIG_PREFIX);
            return;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid mode \"%.200s\"", input + 5);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    if (strncmp(input, "stones ", 7) == 0) {
        int stones = atoi(input + 7);

        if (stones <= 0) {
            renderOutput("Invalid number of stones", CONFIG_PREFIX);
            return;
        }

        if (stones * 12 > UINT8_MAX) {
            char message[256];
            snprintf(message, sizeof(message), "Reducing %d stones per cell to %d to avoid uint8_t overflow", stones, UINT8_MAX / 12);
            renderOutput(message, CONFIG_PREFIX);
            stones = UINT8_MAX / 12;
        }

        config->gameSettings.stones = stones;

        char message[256];
        snprintf(message, sizeof(message), "Updated stones to %d", stones);
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    if (strncmp(input, "autoplay ", 9) == 0) {
        bool originalAutoplay = config->autoplay;

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
            snprintf(message, sizeof(message), "Invalid autoplay \"%.200s\"", input + 9);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    if (strncmp(input, "progress ", 9) == 0) {
        bool original = config->solverConfig.progressBar;

        if (strcmp(input + 9, "true") == 0 || strcmp(input + 9, "1") == 0) {
            config->solverConfig.progressBar = true;
            if (original) {
                renderOutput("Progress bar already enabled", CONFIG_PREFIX);
                return;
            }
            renderOutput("Enabled progress bar", CONFIG_PREFIX);
            return;
        } else if (strcmp(input + 9, "false") == 0 || strcmp(input + 9, "0") == 0) {
            config->solverConfig.progressBar = false;
            if (!original) {
                renderOutput("Progress bar already disabled", CONFIG_PREFIX);
                return;
            }
            renderOutput("Disabled progress bar", CONFIG_PREFIX);
            return;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid progress \"%.200s\"", input + 9);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    if (strncmp(input, "seed ", 5) == 0) {
        int seed = atoi(input + 5);
        config->gameSettings.seed = seed;

        if (seed == 0) {
            seed = time(NULL);
            config->gameSettings.seed = seed;
        }

        char message[256];
        snprintf(message, sizeof(message), "Updated seed to %d", seed);
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    if (strncmp(input, "time ", 5) == 0) {
        double time = atof(input + 5);

        if (time < 0) {
            renderOutput("Invalid time limit", CONFIG_PREFIX);
            return;
        }

        config->solverConfig.timeLimit = time;

        char message[256];
        if (time == 0) {
            snprintf(message, sizeof(message), "Updated time limit to unlimited");
        } else {
            snprintf(message, sizeof(message), "Updated time limit to %g", time);
        }
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    if (strncmp(input, "depth ", 6) == 0) {
        int depth = atoi(input + 6);

        if (depth < 0 || depth > MAX_DEPTH) {
            renderOutput("Invalid depth limit", CONFIG_PREFIX);
            return;
        }

        config->solverConfig.depth = depth;

        char message[256];
        if (depth == 0) {
            snprintf(message, sizeof(message), "Updated depth limit to unlimited");
        } else {
            snprintf(message, sizeof(message), "Updated depth limit to %d", depth);
        }
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    if (strncmp(input, "starting ", 9) == 0) {
        int starting = atoi(input + 9);

        if (starting != 1 && starting != 2) {
            renderOutput("Invalid starting color", CONFIG_PREFIX);
            return;
        }

        config->gameSettings.startColor = starting == 1 ? 1 : -1;

        char message[256];
        snprintf(message, sizeof(message), "Updated starting player to %d", starting);
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    if (strncmp(input, "player ", 7) == 0) {
        int player = atoi(input + 7);

        if (player != 1 && player != 2) {
            renderOutput("Invalid player", CONFIG_PREFIX);
            return;
        }

        if (strncmp(input + 9, "human", 5) == 0) {
            if (player == 1) config->gameSettings.player1 = HUMAN_AGENT;
            else config->gameSettings.player2 = HUMAN_AGENT;
        } else if (strncmp(input + 9, "random", 6) == 0) {
            if (player == 1) config->gameSettings.player1 = RANDOM_AGENT;
            else config->gameSettings.player2 = RANDOM_AGENT;
        } else if (strncmp(input + 9, "ai", 2) == 0) {
            if (player == 1) config->gameSettings.player1 = AI_AGENT;
            else config->gameSettings.player2 = AI_AGENT;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid agent \"%.200s\"", input + 9);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }

        char message[256];
        snprintf(message, sizeof(message), "Updated player %d", player);
        renderOutput(message, CONFIG_PREFIX);
        return;
    }

    if (strncmp(input, "distribution ", 13) == 0) {
        Distribution originalDistribution = config->gameSettings.distribution;

        if (strcmp(input + 13, "uniform") == 0) {
            config->gameSettings.distribution = UNIFORM_DIST;
            if (originalDistribution == UNIFORM_DIST) {
                renderOutput("Distribution already set to uniform", CONFIG_PREFIX);
                return;
            }
            renderOutput("Updated distribution to uniform", CONFIG_PREFIX);
            return;
        } else if (strcmp(input + 13, "random") == 0) {
            config->gameSettings.distribution = RANDOM_DIST;
            if (originalDistribution == RANDOM_DIST) {
                renderOutput("Distribution already set to random", CONFIG_PREFIX);
                return;
            }
            renderOutput("Updated distribution to random", CONFIG_PREFIX);
            return;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid distribution \"%.200s\"", input + 13);
            renderOutput(message, CONFIG_PREFIX);
            return;
        }
    }

    char message[256];
    snprintf(message, sizeof(message), "Unknown command \"%.200s\"", input);
    renderOutput(message, CONFIG_PREFIX);
}
