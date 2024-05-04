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
        configBoardRand(board, config->stones);
    }
    board->color = config->startColor;
}

void quitGame() {
    exit(0);
}

void updateCell(Board* board, int player, int idx, int value) {
    int currentSum = 0;

    int cellIndex = player == -1 ? 13 - idx : idx - 1;

    for (int i = 0; i < 14; i++) {
        if (i == cellIndex) {
            continue;
        }
        currentSum += board->cells[i];
    }

    int futureSum = currentSum + value;

    if (futureSum > UINT8_MAX) {
        char* message = malloc(256);
        asprintf(&message, "Can't update cell from %d to %d. Risk of overflow (%d / %d)", board->cells[cellIndex], value, futureSum, UINT8_MAX);
        renderOutput(message, PLAY_PREFIX);
        free(message);
        return;
    }

    renderOutput("Updated cell", PLAY_PREFIX);
    board->cells[cellIndex] = value;
    return;
}
