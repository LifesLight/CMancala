/**
 * Copyright (c) Alexander Kurtz 2023
 */

#include "render.h"

void renderBoard(const Board *board) {
    // Print indices
    printf("IDX:  ");
    for (int i = 1; i < LBOUND_P2; i++) {
        printf("%d   ", i);
    }
    printf("\n");

    // Top header
    printf("    %s%s", TL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, ET, HL);
    }
    printf("%s%s%s\n", HL, HL, TR);

    // Upper row
    printf("%s%s%s%s%s", TL, HL, HL, HL, ER);
    for (int i = HBOUND_P2; i > LBOUND_P2; --i) {
        printf("%3d%s", board->cells[i], VL);
    }
    printf("%3d%s%s%s%s%s\n", board->cells[LBOUND_P2], EL, HL, HL, HL, TR);

    // Middle separator
    printf("%s%3d%s%s", VL, board->cells[SCORE_P2], EL, HL);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%s%s%s%s", HL, HL, CR, HL);
    }
    printf("%s%s%s%3d%s\n", HL, HL, ER, board->cells[SCORE_P1], VL);

    // Lower row
    printf("%s%s%s%s%s", BL, HL, HL, HL, ER);
    for (int i = LBOUND_P1; i < HBOUND_P1; ++i) {
        printf("%3d%s", board->cells[i], VL);
    }
    printf("%3d%s%s%s%s%s\n", board->cells[HBOUND_P1], EL, HL, HL, HL, BR);

    // Bottom footer
    printf("    %s%s", BL, HL);
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

void renderConfigHelp() {
    printf("%s%sCommands:\n", CONFIG_PREFIX, OUTPUT_PREFIX);
    printf("%s%s  start                            : Start the game\n", CONFIG_PREFIX, OUTPUT_PREFIX);
    printf("%s%s  stones [number > 0]              : Set number of stones per pit (default: 4)\n", CONFIG_PREFIX, OUTPUT_PREFIX);
    printf("%s%s  distribution [uniform | random]  : Configure distribution of stones \n", CONFIG_PREFIX, OUTPUT_PREFIX);
    printf("%s%s  seed [number]                    : Set seed for random distribution (if not specified random)\n", CONFIG_PREFIX, OUTPUT_PREFIX);
    printf("%s%s  time [number >= 0]               : Set time limit for AI in seconds, if 0 unlimited\n", CONFIG_PREFIX, OUTPUT_PREFIX);
    printf("%s%s  depth [number >= 0]              : Set depth limit for AI, if 0 not depth limited\n", CONFIG_PREFIX, OUTPUT_PREFIX);
    printf("%s%s  starting [human | ai]            : Configure starting player\n", CONFIG_PREFIX, OUTPUT_PREFIX);
    printf("%s%s  display                          : Display current configuration\n", CONFIG_PREFIX, OUTPUT_PREFIX);
    printf("%s%s  help                             : Print this help message\n", CONFIG_PREFIX, OUTPUT_PREFIX);
    printf("%s%s  quit                             : Quit the application\n", CONFIG_PREFIX, OUTPUT_PREFIX);
}

void renderOutput(const char* message, const char* prefix) {
    printf("%s%s%s\n", prefix, OUTPUT_PREFIX, message);
}
