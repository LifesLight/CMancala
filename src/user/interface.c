/**
 * Copyright (c) Alexander Kurtz 2024
 */

#include "user/interface.h"


void startInterface() {
    // Print welcome message
    renderWelcome();

    // Global config struct
    SolverConfig solverConfig = {
        .solver = GLOBAL_SOLVER,
        .depth = 0,
        .timeLimit = 5.0,
        .goodEnough = 0
    };

    GameSettings gameSettings = {
        .stones = 4,
        .distribution = UNIFORM_DIST,
        .seed = time(NULL),
        .startColor = 1,
        .player1 = HUMAN_AGENT,
        .player2 = AI_AGENT,
        .moveFunction = CLASSIC_MOVE
    };

    Config config = {
        .autoplay = true,
        .gameSettings = gameSettings,
        .solverConfig = solverConfig
    };

    // Global loop
    while (true) {
        bool requestedStart = false;
        handleConfigInput(&requestedStart, &config);

        if (requestedStart) {
            srand(config.gameSettings.seed);
            startGameHandling(&config);
        }
    }
}
