/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "interface.h"


void startInterface() {
    // Print welcome message
    renderWelcome();

    // Global config struct
    Config config = {
        .stones = 4,
        .distribution = UNIFORM_DIST,
        .seed = time(NULL),
        .timeLimit = 5.0,
        .depth = 0,
        .startColor = 1,
        .autoplay = true,
        .player1 = AI_AGENT,
        .player2 = HUMAN_AGENT
    };

    // Global loop
    while (true) {
        bool requestedStart = false;
        handleConfigInput(&requestedStart, &config);

        if (requestedStart) {
            startGameHandling(&config);
        }
    }
}
