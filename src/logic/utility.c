/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "utility.h"


void quitGame() {
    exit(0);
}

void trimSpaces(char *str) {
    char *end = str + strlen(str);
    while (end > str && isspace((unsigned char)*(end - 1))) end--;
    *end = 0;

    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
}

void getInput(char *input, const char *prefix) {
    bool isInteractive = isatty(STDIN_FILENO);

    while (true) {
        printf("%s%s", prefix, INPUT_PREFIX);
        fflush(stdout);

        if (fgets(input, 256, stdin) == NULL) {
            if (feof(stdin)) {
                if (isInteractive) {
                    clearerr(stdin);
                } else {
                    printf("End of input\n");
                    quitGame();
                }
            } else {
                renderOutput("Error reading input", prefix);
                exit(EXIT_FAILURE);
            }
        } else {
            trimSpaces(input);
            if (strlen(input) == 0) continue;

            if (strcmp(input, "quit") == 0 || strcmp(input, "q") == 0) {
                quitGame();
            }

            if (!isInteractive) {
                printf("%s\n", input);
            }

            break;
        }
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
        char message[256];
        snprintf(message, sizeof(message), "Can't update cell from %d to %d. Risk of overflow (%d / %d)", board->cells[cellIndex], value, futureSum, UINT8_MAX);
        renderOutput(message, PLAY_PREFIX);
        return;
    }

    renderOutput("Updated cell", PLAY_PREFIX);
    board->cells[cellIndex] = value;
    return;
}
