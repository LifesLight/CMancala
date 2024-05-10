/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "user/render.h"


void renderCustomBoard(const int32_t *cells, const int8_t color, const char* prefix, const Config* config) {
    char* playerDescriptor;
    if (color == 1) {
        switch (config->player1) {
            case HUMAN_AGENT:
                playerDescriptor = "Player";
                break;
            case AI_AGENT:
                playerDescriptor = "AI";
                break;
            case RANDOM_AGENT:
                playerDescriptor = "Random";
                break;
        }
    } else {
        switch (config->player2) {
            case HUMAN_AGENT:
                playerDescriptor = "Player";
                break;
            case AI_AGENT:
                playerDescriptor = "AI";
                break;
            case RANDOM_AGENT:
                playerDescriptor = "Random";
                break;
        }
    }

    // Print indices
    printf("%s%sIDX:  ", prefix, OUTPUT_PREFIX);
    for (int i = 1; i < LBOUND_P2; i++) {
        printf("%d   ", i);
    }
    printf("\n");

    // Top header
    printf("%s%s    %s%s", prefix, OUTPUT_PREFIX, TL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, ET, HL);
    }
    printf("%s%s%s\n", HL, HL, TR);

    // Upper row
    printf("%s%s%s%s%s%s%s", prefix, OUTPUT_PREFIX, TL, HL, HL, HL, ER);
    for (int i = HBOUND_P2; i > LBOUND_P2; --i) {
        if (cells[i] == INT32_MIN)
            printf(" X %s", VL);
        else
            printf("%3d%s", cells[i], VL);
    }
    if (cells[LBOUND_P2] == INT32_MIN)
        printf(" X %s%s%s%s%s", EL, HL, HL, HL, TR);
    else
        printf("%3d%s%s%s%s%s", cells[LBOUND_P2], EL, HL, HL, HL, TR);

    if (color == -1) {
        printf("  %s%s", PLAYER_INDICATOR, playerDescriptor);
    }
    printf("\n");

    // Middle separator
    printf("%s%s%s%3d%s%s", prefix, OUTPUT_PREFIX, VL, cells[SCORE_P2], EL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, CR, HL);
    }
    printf("%s%s%s%3d%s\n", HL, HL, ER, cells[SCORE_P1], VL);

    // Lower row
    printf("%s%s%s%s%s%s%s", prefix, OUTPUT_PREFIX, BL, HL, HL, HL, ER);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        if (cells[i] == INT32_MIN)
            printf(" X %s", VL);
        else
            printf("%3d%s", cells[i], VL);
    }
    if (cells[HBOUND_P1] == INT32_MIN)
        printf(" X %s%s%s%s%s", EL, HL, HL, HL, BR);
    else
        printf("%3d%s%s%s%s%s", cells[HBOUND_P1], EL, HL, HL, HL, BR);

    if (color == 1) {
        printf("  %s%s", PLAYER_INDICATOR, playerDescriptor);
    }
    printf("\n");

    // Bottom footer
    printf("%s%s    %s%s", prefix, OUTPUT_PREFIX, BL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, EB, HL);
    }
    printf("%s%s%s\n", HL, HL, BR);
}

void renderBoard(const Board *board, const char* prefix, const Config* config) {
    int cells[14];
    for (int i = 0; i < 14; ++i) {
        cells[i] = board->cells[i];
    }

    renderCustomBoard(cells, board->color, prefix, config);
}

void renderWelcome() {
    printf("+-----------------------------------------+\n");
    printf("| %sWelcome to CMancala!                 |\n", OUTPUT_PREFIX);
    printf("| %sType 'help' for a list of commands   |\n", OUTPUT_PREFIX);
    printf("|                                         |\n");
    printf("| (c) Alexander Kurtz 2024                |\n");
    printf("+-----------------------------------------+\n");
    printf("\n");
}

void renderOutput(const char* message, const char* prefix) {
    printf("%s%s%s\n", prefix, OUTPUT_PREFIX, message);
}
