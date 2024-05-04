/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "render.h"


void renderBoard(const Board *board, const char* prefix) {
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
        printf("%3d%s", board->cells[i], VL);
    }
    printf("%3d%s%s%s%s%s\n", board->cells[LBOUND_P2], EL, HL, HL, HL, TR);

    // Middle separator
    printf("%s%s%s%3d%s%s", prefix, OUTPUT_PREFIX, VL, board->cells[SCORE_P2], EL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, CR, HL);
    }
    printf("%s%s%s%3d%s\n", HL, HL, ER, board->cells[SCORE_P1], VL);

    // Lower row
    printf("%s%s%s%s%s%s%s", prefix, OUTPUT_PREFIX, BL, HL, HL, HL, ER);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%3d%s", board->cells[i], VL);
    }
    printf("%3d%s%s%s%s%s\n", board->cells[HBOUND_P1], EL, HL, HL, HL, BR);

    // Bottom footer
    printf("%s%s    %s%s", prefix, OUTPUT_PREFIX, BL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, EB, HL);
    }
    printf("%s%s%s\n", HL, HL, BR);
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
