/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "utility.h"

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
    if (config->distribution == UNIFORM_DIST) {
        configBoard(board, config->stones);
    } else {
        configBoardRand(board, config->stones, config->seed);
    }
    board->color = config->startColor;
}

void quitGame() {
    exit(0);
}
