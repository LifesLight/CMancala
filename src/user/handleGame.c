/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "handleGame.h"

void renderCheatHelp() {
    renderOutput("Commands:", CHEAT_PREFIX);
    renderOutput("  step                             : Step to the next turn", CHEAT_PREFIX);
    #ifdef ENCODING
    renderOutput("  encode                           : Encode the current board", CHEAT_PREFIX);
    renderOutput("  load [encoding]                  : Load the board from the encoding", CHEAT_PREFIX);
    #endif
    renderOutput("  undo                             : Undo the last move", CHEAT_PREFIX);
    renderOutput("  switch                           : Switch the next player", CHEAT_PREFIX);
    renderOutput("  edit [player] [idx] [value]      : Edit cell value", CHEAT_PREFIX);
    renderOutput("  render                           : Render the current board", CHEAT_PREFIX);
    renderOutput("  analyze [(depth)]                : Run a full analysis on the board", CHEAT_PREFIX);
    renderOutput("  last                             : Fetch the last moves metadata", CHEAT_PREFIX);
    renderOutput("  trace                            : Compute move trace of the last move (requires cached evaluation)", CHEAT_PREFIX);
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

    char input[256];
    getInput(input, CHEAT_PREFIX);


    // Check for request to step
    if (strcmp(input, "step") == 0) {
        *requestContinue = true;
        return;
    }

    // Check for request to undo
    if (strcmp(input, "undo") == 0) {
        if (context->lastMove == -1) {
            renderOutput("No move to undo", CHEAT_PREFIX);
            return;
        }

        copyBoard(context->lastBoard, context->board);
        context->lastMove = -1;
        context->lastEvaluation = INT32_MAX;
        context->lastDepth = 0;
        context->lastSolved = false;
        context->gameOver = false;
        renderOutput("Undid last move", CHEAT_PREFIX);
        return;
    }

    // Check for request to analyze
    if (strncmp(input, "analyze", 7) == 0) {
        int depth = 14;

        // Check for custom depth
        if (strlen(input) > 7) {
            depth = atoi(input + 8);
        }

        if (depth < 1) {
            renderOutput("Invalid depth", CHEAT_PREFIX);
            return;
        }

        // Manually
        int distribution[6];
        bool solved = true;
        negamaxRootWithDistribution(context->board, depth, distribution, &solved);

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

        renderCustomBoard(renderCells, context->board->color, CHEAT_PREFIX, context->config);

        return;
    }

    #ifdef ENCODING
    // Check for request to hash
    if (strcmp(input, "encode") == 0) {
        __uint128_t code = encodeBoard(context->board);
        char message[256];
        // Format the hash as hexadecimal
        snprintf(message, sizeof(message), "%016llx%016llx", 
                (unsigned long long)(code >> 64), 
                (unsigned long long)(code & 0xFFFFFFFFFFFFFFFFULL));

        // Remove first 6 characters
        char temp[256];
        strncpy(temp, message + 6, sizeof(temp));
        snprintf(message, sizeof(message), "Code: %s", temp);

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

        // Prepare to parse the hash as hexadecimal
        uint64_t high = 0, low = 0;

        // Remove leading spaces
        while (input[5] == ' ') {
            sprintf(input, "%s", input + 1);
        }

        // Add 6 trailing zeros to the input
        char temp[256];
        snprintf(temp, sizeof(temp), "000000%s", input + 5);

        // Read in temp as hexadecimal
        int parsedCount = sscanf(temp, "%16llx%16llx", (unsigned long long int *)&high, (unsigned long long int *)&low);
        __uint128_t code = ((__uint128_t)high << 64) | low;

        // Check if the parsing was successful and exactly two 64-bit parts were read
        if (parsedCount != 2) {
            renderOutput("Invalid code", CHEAT_PREFIX);
            return;
        }

        // Save previous board
        copyBoard(context->board, context->lastBoard);
        context->lastEvaluation = INT32_MAX;

        // Load board
        decodeBoard(context->board, code);
        renderOutput("Loaded board", CHEAT_PREFIX);
        return;
    }
    #endif

    // Check for request to switch
    if (strcmp(input, "switch") == 0) {
        context->board->color = context->board->color * -1;
        renderOutput("Switched player", CHEAT_PREFIX);
        return;
    }

    // Check for request to trace
    if (strcmp(input, "trace") == 0) {
        if (context->lastMove == -1) {
            renderOutput("No move to trace", CHEAT_PREFIX);
            return;
        }

        if (context->lastEvaluation == INT32_MAX) {
            renderOutput("No evaluation to trace", CHEAT_PREFIX);
            return;
        }

        NegamaxTrace trace = negamaxWithTrace(context->lastBoard, context->lastEvaluation - 1, context->lastEvaluation + 1, context->lastDepth);

        int stepsToWin = 0;
        for (int i = context->lastDepth - 1; i >= 0; i--) {
            if (trace.moves[i] == -1) {
                break;
            }

            stepsToWin++;
        }

        int width = snprintf(NULL, 0, "%d", stepsToWin);
        for (int i = context->lastDepth - 1; i >= 0; i--) {
            char message[256];

            int move = trace.moves[i];

            if (move == -1) {
                break;
            }

            if (move > 5) {
                move = 13 - move;
                snprintf(message, sizeof(message), "[%*d|%s] >> %d", width, context->lastDepth - i, "Player 2", move);
            } else {
                move = move + 1;
                snprintf(message, sizeof(message), "[%*d|%s] >> %d", width, context->lastDepth - i, "Player 1", move);
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
        renderBoard(context->board, CHEAT_PREFIX, context->config);
        return;
    }

    // Check for last
    if (strcmp(input, "last") == 0) {
        char message[256];
        snprintf(message, sizeof(message), "Metadata:");
        renderOutput(message, CHEAT_PREFIX);

        if (context->lastMove != -1) {
            snprintf(message, sizeof(message), "  Move: %d", context->lastMove > 5 ? 13 - context->lastMove : context->lastMove + 1);
            renderOutput(message, CHEAT_PREFIX);
        }

        if (context->lastEvaluation != INT32_MAX) {
            snprintf(message, sizeof(message), "  Depth: %d", context->lastDepth);
            renderOutput(message, CHEAT_PREFIX);

            snprintf(message, sizeof(message), "  Evaluation: %d", context->lastEvaluation);
            renderOutput(message, CHEAT_PREFIX);

            if (context->lastSolved) {
                #ifdef GREEDY_SOLVING
                snprintf(message, sizeof(message), "  Solved: true (greedy)");
                #else
                snprintf(message, sizeof(message), "  Solved: true");
                #endif
            } else {
                #ifdef GREEDY_SOLVING
                snprintf(message, sizeof(message), "  Solved: false (greedy)");
                #else
                snprintf(message, sizeof(message), "  Solved: false");
                #endif
            }
            renderOutput(message, CHEAT_PREFIX);

            #ifdef TRACK_THROUGHPUT
            int64_t totalNodes = context->lastNodes;
            double totalTime = context->lastTime;
            double nodesPerSecond = totalNodes / totalTime;
            snprintf(message, sizeof(message), "  Throughput: %f million nodes/s", nodesPerSecond / 1000000);
            renderOutput(message, CHEAT_PREFIX);
            #endif
        }

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
                return;
            }

            renderOutput("Enabled autoplay", CHEAT_PREFIX);
            return;
        } else if (strcmp(input + 9, "false") == 0 || strcmp(input + 9, "0") == 0) {
            context->config->autoplay = false;
            if (!originalAutoplay) {
                renderOutput("Autoplay already disabled", CHEAT_PREFIX);
                return;
            }

            renderOutput("Disabled autoplay", CHEAT_PREFIX);
            return;
        } else {
            char message[256];
            snprintf(message, sizeof(message), "Invalid autoplay \"%s\"", input + 9);
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
    snprintf(message, sizeof(message), "Unknown command: \"%s\". Type \"help\" to get all current commands", input);
    renderOutput(message, CHEAT_PREFIX);
    return;
}

void startGameHandling(Config* config) {
    // Initialize board
    Board board;
    initializeBoardFromConfig(&board, config);

    setMoveFunction(config->moveFunction);

    // Make game context
    Context context = {
        .board = &board,
        .lastBoard = malloc(sizeof(Board)),
        .config = config,
        .lastEvaluation = INT32_MAX,
        .lastMove = -1,
        .lastDepth = 0,
        .lastSolved = false,
        .gameOver = false
    };

    // Start game loop
    bool requestedConfig = false;
    bool requestedContinue = config->autoplay ? true : false;

    while(!requestedConfig) {
        while(requestedContinue && !context.gameOver) {
            bool requestedMenu = false;

            stepGame(&requestedMenu, &context);

            if (!config->autoplay || requestedMenu) {
                requestedContinue = false;
                break;
            }

            if (isBoardPlayerOneEmpty(&board) || isBoardPlayerTwoEmpty(&board)) {
                context.gameOver = true;
                requestedContinue = false;

                renderBoard(&board, PLAY_PREFIX, config);

                int score1 = board.cells[SCORE_P1];
                int score2 = board.cells[SCORE_P2];

                char message[256];
                if (score1 > score2) {
                    snprintf(message, sizeof(message), "Player 1 wins");
                } else if (score1 < score2) {
                    snprintf(message, sizeof(message), "Player 2 wins");
                } else {
                    snprintf(message, sizeof(message), "Draw");
                }

                snprintf(message, sizeof(message), "%s, score: %d - %d", message, score1, score2);
                renderOutput(message, PLAY_PREFIX);

                break;
            }
        }

        handleGameInput(&requestedConfig, &requestedContinue, &context);
    }
}
