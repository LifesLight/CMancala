/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "user/handleGame.h"

static int safe_int_max_for_buffer(size_t bufsize, const char *prefix) {
    int overhead = (int)strlen(prefix) + 2;
    int max = (int)bufsize - 1 - overhead;
    return max > 0 ? max : 0;
}

void renderCheatHelp() {
    renderOutput("Commands:", CHEAT_PREFIX);
    renderOutput("  step                             : Step to the next turn", CHEAT_PREFIX);
    renderOutput("  encode                           : Encode the current board", CHEAT_PREFIX);
    renderOutput("  load [encoding]                  : Load the board from the encoding", CHEAT_PREFIX);
    renderOutput("  undo                             : Undo the last move", CHEAT_PREFIX);
    renderOutput("  switch                           : Switch the next player", CHEAT_PREFIX);
    renderOutput("  edit [player] [idx] [value]      : Edit cell value", CHEAT_PREFIX);
    renderOutput("  render                           : Render the current board", CHEAT_PREFIX);
    renderOutput("  analyze --solver --depth --clip  : Run analysis on the board, solver, depth and clip can be specified", CHEAT_PREFIX);
    renderOutput("  last                             : Fetch the last moves metadata", CHEAT_PREFIX);
    renderOutput("  cache                            : Fetch the cache stats", CHEAT_PREFIX);
    renderOutput("  trace                            : Compute move trace of the last move (requires cached evaluation)", CHEAT_PREFIX);
    renderOutput("  autoplay [true|false]            : If enabled the game loop will automatically continue", CHEAT_PREFIX);
    renderOutput("  config                           : Return the config menu. Will discard the current game", CHEAT_PREFIX);
    renderOutput("  help                             : Print this help", CHEAT_PREFIX);
    renderOutput("  quit                             : Quit the application", CHEAT_PREFIX);
}

void handleGameInput(bool* requestedConfig, bool* requestContinue, Context* context) {
    char input[256];
    getInput(input, CHEAT_PREFIX);

    // Check for request to step
    if (strcmp(input, "step") == 0) {
        *requestContinue = true;
        return;
    }

    // Check for request to undo
    if (strcmp(input, "undo") == 0) {
        if (context->metadata.lastMove == -1) {
            renderOutput("No move to undo", CHEAT_PREFIX);
            return;
        }

        copyBoard(context->lastBoard, context->board);
        context->metadata.lastMove = -1;
        context->metadata.lastEvaluation = INT32_MAX;
        context->metadata.lastDepth = 0;
        context->metadata.lastSolved = false;
        renderOutput("Undid last move", CHEAT_PREFIX);
        return;
    }

    // Check for request to analyze
    if (strncmp(input, "analyze", 7) == 0) {
        char internalInput[256];
        snprintf(internalInput, sizeof(internalInput), "%s", input + 8);

        // Default depth
        SolverConfig solveConfig = context->config.solverConfig;

        // Find --solver, --depth, and --clip in the input string
        char *solverPoint = strstr(internalInput, "--solver");
        char *depthPoint = strstr(internalInput, "--depth");
        char *clipPoint = strstr(internalInput, "--clip");

        // Parse and set the solver configuration
        if (solverPoint != NULL) {
            solverPoint += 9;
            char *nextSpace = strchr(solverPoint, ' ');
            if (nextSpace != NULL) *nextSpace = '\0';

            if (strcmp(solverPoint, "global") == 0) {
                solveConfig.solver = GLOBAL_SOLVER;
            } else if (strcmp(solverPoint, "local") == 0) {
                solveConfig.solver = LOCAL_SOLVER;
            } else {
                renderOutput("Invalid solver", CHEAT_PREFIX);
                return;
            }
        }

        // Parse and set the depth configuration
        if (depthPoint != NULL) {
            depthPoint += 8;
            char *nextSpace = strchr(depthPoint, ' ');
            if (nextSpace != NULL) *nextSpace = '\0';

            char *endPtr;
            int parsedDepth = (int)strtol(depthPoint, &endPtr, 10);
            if (depthPoint == endPtr) {
                renderOutput("Invalid depth value", CHEAT_PREFIX);
                return;
            }
            solveConfig.depth = parsedDepth;
        }

        // Check clip true false
        if (clipPoint != NULL) {
            clipPoint += 7;
            char *nextSpace = strchr(clipPoint, ' ');
            if (nextSpace != NULL) *nextSpace = '\0';

            if (strcmp(clipPoint, "true") == 0) {
                solveConfig.clip = true;
            } else if (strcmp(clipPoint, "false") == 0) {
                solveConfig.clip = false;
            } else {
                renderOutput("Invalid clip value", CHEAT_PREFIX);
                return;
            }
        }

        if (solveConfig.depth == 0) {
            solveConfig.depth = 16;
        }

        // Manually
        int distribution[6];
        bool solved = false;

        distributionRoot(context->board, distribution, &solved, &solveConfig);

        int renderCells[14];
        for (int i = 0; i < 14; i++) {
            renderCells[i] = context->board->cells[i];
        }
        if (context->board->color == 1) {
            for (int i = 0; i < 6; i++) {
                renderCells[i] = distribution[i];
            }
        } else {
            for (int i = 0; i < 6; i++) {
                renderCells[i + 7] = distribution[i];
            }
        }

        renderCustomBoard(renderCells, context->board->color, CHEAT_PREFIX, &context->config.gameSettings);

        switch (solveConfig.solver) {
            case GLOBAL_SOLVER:
                if (solveConfig.clip) {
                    renderOutput("Solver: Global Clipped", CHEAT_PREFIX);
                } else {
                    renderOutput("Solver: Global", CHEAT_PREFIX);
                }
                break;
            case LOCAL_SOLVER:
                if (solveConfig.clip) {
                    renderOutput("Solver: Local Clipped", CHEAT_PREFIX);
                } else {
                    renderOutput("Solver: Local", CHEAT_PREFIX);
                }
                break;
        }

        if (solved) {
            renderOutput("Solved", CHEAT_PREFIX);
        }

        char message[256];
        snprintf(message, sizeof(message), "Depth: %d", solveConfig.depth);
        renderOutput(message, CHEAT_PREFIX);

        return;
    }

    // Check for request to hash
    if (strcmp(input, "encode") == 0) {
        char* code = encodeBoard(context->board);

        char message[256];

        int allowed = safe_int_max_for_buffer(sizeof(message), "Code ");
        snprintf(message, sizeof(message), "Code \"%.*s\"", allowed, code);

        free(code);
        renderOutput(message, CHEAT_PREFIX);
        return;
    }

    // Check for request to load
    if (strncmp(input, "load ", 5) == 0) {
        // Check for valid input length
        if (strlen(input) < 7) {
            renderOutput("Invalid load command", CHEAT_PREFIX);
            return;
        }

        char *payload = input + 5;
        while (*payload == ' ') payload++;
        memmove(input + 5, payload, strlen(payload) + 1);

        char temp[256];
        snprintf(temp, sizeof(temp), "00000%.*s", (int)(sizeof(temp) - 6), input + 5);

        // Save previous board
        copyBoard(context->board, context->lastBoard);
        context->metadata.lastEvaluation = INT32_MAX;

        // Load board
        bool success = decodeBoard(context->board, temp);

        if (!success) {
            renderOutput("Invalid code", CHEAT_PREFIX);
            return;
        }

        renderOutput("Loaded board", CHEAT_PREFIX);
        return;
    }

    // Check for request to switch
    if (strcmp(input, "switch") == 0) {
        context->board->color = context->board->color * -1;
        renderOutput("Switched player", CHEAT_PREFIX);
        return;
    }

    // Check for request to trace
    if (strcmp(input, "trace") == 0) {
        if (context->metadata.lastMove == -1) {
            renderOutput("No move to trace", CHEAT_PREFIX);
            return;
        }

        if (context->metadata.lastEvaluation == INT32_MAX) {
            renderOutput("No evaluation to trace", CHEAT_PREFIX);
            return;
        }

        NegamaxTrace trace = traceRoot(context->lastBoard, context->metadata.lastEvaluation - 1, context->metadata.lastEvaluation + 1, context->metadata.lastDepth);

        int stepsToWin = 0;
        for (int i = context->metadata.lastDepth - 1; i >= 0; i--) {
            if (trace.moves[i] == -1) {
                break;
            }

            stepsToWin++;
        }

        int width = snprintf(NULL, 0, "%d", stepsToWin);
        for (int i = context->metadata.lastDepth - 1; i >= 0; i--) {
            char message[256];

            int move = trace.moves[i];

            if (move == -1) {
                break;
            }

            if (move > 5) {
                move = 13 - move;
                snprintf(message, sizeof(message), "[%*d|%s] >> %d", width, context->metadata.lastDepth - i, "Player 2", move);
            } else {
                move = move + 1;
                snprintf(message, sizeof(message), "[%*d|%s] >> %d", width, context->metadata.lastDepth - i, "Player 1", move);
            }

            renderOutput(message, CHEAT_PREFIX);
        }

        free(trace.moves);
        return;
    }

    // Check for request to edit
    if (strncmp(input, "edit ", 5) == 0) {
        // Check for valid input
        if (strlen(input) < 7) {
            renderOutput("Invalid edit command", CHEAT_PREFIX);
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
            return;
        }

        // Parse idx
        int idx = atoi(input + 7);
        if (idx < 1 || idx > 6) {
            renderOutput("Invalid idx", CHEAT_PREFIX);
            return;
        }

        // Parse value
        int value = atoi(input + 9);
        if (value < 0) {
            renderOutput("Invalid value", CHEAT_PREFIX);
            return;
        }

        // Update cell
        updateCell(context->board, player, idx, value);
        return;
    }

    // If empty just return
    if (strcmp(input, "") == 0) {
        return;
    }

    // Check for render
    if (strcmp(input, "render") == 0) {
        renderBoard(context->board, CHEAT_PREFIX, &context->config.gameSettings);
        return;
    }

    // Check for last
    if (strcmp(input, "last") == 0) {
        char message[256];
        snprintf(message, sizeof(message), "Metadata:");
        renderOutput(message, CHEAT_PREFIX);

        if (context->metadata.lastMove != -1) {
            snprintf(message, sizeof(message), "  Move: %d", context->metadata.lastMove > 5 ? 13 - context->metadata.lastMove : context->metadata.lastMove + 1);
            renderOutput(message, CHEAT_PREFIX);
        }

        if (context->metadata.lastEvaluation != INT32_MAX) {
            snprintf(message, sizeof(message), "  Depth: %d", context->metadata.lastDepth);
            renderOutput(message, CHEAT_PREFIX);

            snprintf(message, sizeof(message), "  Evaluation: %d", context->metadata.lastEvaluation);
            renderOutput(message, CHEAT_PREFIX);

            if (context->metadata.lastSolved) {
                snprintf(message, sizeof(message), "  Solved: true");
            } else {
                snprintf(message, sizeof(message), "  Solved: false");
            }
            renderOutput(message, CHEAT_PREFIX);

            int64_t totalNodes = context->metadata.lastNodes;

            snprintf(message, sizeof(message), "  Total nodes: %.3f million", (double)totalNodes / 1000000.0);
            renderOutput(message, CHEAT_PREFIX);

            double totalTime = context->metadata.lastTime;
            if (totalTime <= 0.0) {
                snprintf(message, sizeof(message), "  Total time:  N/A");
                renderOutput(message, CHEAT_PREFIX);

                snprintf(message, sizeof(message), "  Throughput:  N/A");
                renderOutput(message, CHEAT_PREFIX);
            } else {
                snprintf(message, sizeof(message), "  Total time:  %f seconds", totalTime);
                renderOutput(message, CHEAT_PREFIX);

                double nodesPerSecond = totalNodes / totalTime;
                snprintf(message, sizeof(message), "  Throughput:  %f million nodes/s", nodesPerSecond / 1000000.0);
                renderOutput(message, CHEAT_PREFIX);
            }
        }

        return;
    }

    // Check for cache
    if (strcmp(input, "cache") == 0) {
        if (getCacheSize() == 0) {
            renderOutput("  Cache disabled", CHEAT_PREFIX);
            return;
        }
        renderCacheStats();
        return;
    }

    // Check for autoplay
    if (strncmp(input, "autoplay ", 9) == 0) {
        // Save original autoplay
        bool originalAutoplay = context->config.autoplay;

        // Check if valid autoplay
        if (strcmp(input + 9, "true") == 0 || strcmp(input + 9, "1") == 0) {
            context->config.autoplay = true;
            if (originalAutoplay) {
                renderOutput("Autoplay already enabled", CHEAT_PREFIX);
                return;
            }

            renderOutput("Enabled autoplay", CHEAT_PREFIX);
            return;
        } else if (strcmp(input + 9, "false") == 0 || strcmp(input + 9, "0") == 0) {
            context->config.autoplay = false;
            if (!originalAutoplay) {
                renderOutput("Autoplay already disabled", CHEAT_PREFIX);
                return;
            }

            renderOutput("Disabled autoplay", CHEAT_PREFIX);
            return;
        } else {
            char message[256];

            const char *prefix = "Invalid autoplay ";
            int allowed = safe_int_max_for_buffer(sizeof(message), prefix);
            snprintf(message, sizeof(message), "%s\"%.*s\"", prefix, allowed, input + 9);
            renderOutput(message, CHEAT_PREFIX);
            return;
        }
    }

    // Check for request to config
    if (strcmp(input, "config") == 0) {
        *requestedConfig = true;
        return;
    }

    // Check for help
    if (strcmp(input, "help") == 0) {
        renderCheatHelp();
        return;
    }

    char message[256];
    /* safely print unknown command */
    const char *unknown_prefix = "Unknown command: ";
    int allowed = safe_int_max_for_buffer(sizeof(message), unknown_prefix);
    snprintf(message, sizeof(message), "%s\"%.*s\"", unknown_prefix, allowed, input);
    renderOutput(message, CHEAT_PREFIX);
    return;
}

void startGameHandling(Config config) {
    // Initialize board
    Board board;
    initializeBoardFromConfig(&board, &config);

    setMoveFunction(config.gameSettings.moveFunction);

    Metadata metadata = {
        .lastEvaluation = INT32_MAX,
        .lastMove = -1,
        .lastDepth = 0,
        .lastSolved = false,
        .lastTime = 0,
        .lastNodes = 0,
    };

    // Make game context
    Context context = {
        .board = &board,
        .lastBoard = malloc(sizeof(Board)),
        .config = config,
        .metadata = metadata,
    };

    // Start game loop
    bool requestedConfig = false;
    bool requestedContinue = context.config.autoplay ? true : false;

    while(!requestedConfig) {
        if (isBoardTerminal(context.board)) {
            requestedContinue = false;
        }

        while(requestedContinue) {
            bool requestedMenu = false;

            stepGame(&requestedMenu, &context);

            // Only inform about winner if a player made the game end
            if (isBoardTerminal(context.board)) {
                requestedContinue = false;

                renderBoard(&board, PLAY_PREFIX, &context.config.gameSettings);

                int score1 = board.cells[SCORE_P1];
                int score2 = board.cells[SCORE_P2];

                char message[256];
                char temp[256];
                if (score1 > score2) {
                    snprintf(temp, sizeof(temp), "Player 1 wins");
                } else if (score1 < score2) {
                    snprintf(temp, sizeof(temp), "Player 2 wins");
                } else {
                    snprintf(temp, sizeof(temp), "Draw");
                }

                snprintf(message, sizeof(message), "%.200s, score: %d - %d", temp, score1, score2);
                renderOutput(message, PLAY_PREFIX);

                break;
            }

            if (!context.config.autoplay || requestedMenu) {
                requestedContinue = false;
                break;
            }
        }

        handleGameInput(&requestedConfig, &requestedContinue, &context);
    }
}
