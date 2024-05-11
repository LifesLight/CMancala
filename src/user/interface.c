/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "user/interface.h"


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
        .player1 = HUMAN_AGENT,
        .player2 = AI_AGENT,
        .moveFunction = CLASSIC_MOVE,
        .solver = GLOBAL_SOLVER,
        .quickSolverCutoff = 1
    };

    // Global loop
    while (true) {
        bool requestedStart = false;
        handleConfigInput(&requestedStart, &config);

        if (requestedStart) {
            srand(config.seed);
            startGameHandling(&config);
        }
    }
}
