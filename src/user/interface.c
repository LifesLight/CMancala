// src/user/interface_cli.c
#include "user/interface.h"

void startInterface() {
    renderWelcome();
    setMoveFunction(CLASSIC_MOVE);

    SolverConfig solverConfig = {
        .solver = LOCAL_SOLVER,
        .depth = 0,
        .timeLimit = 5.0,
        .clip = false,
        .compressCache = AUTO,
        .progressBar = true
    };

    GameSettings gameSettings = {
        .stones = 4,
        .distribution = UNIFORM_DIST,
        .seed = time(NULL),
        .startColor = 1,
        .player1 = HUMAN_AGENT,
        .player2 = AI_AGENT
    };

    Config config = {
        .autoplay = true,
        .gameSettings = gameSettings,
        .solverConfig = solverConfig
    };

    while (true) {
        bool requestedStart = false;
        handleConfigInput(&requestedStart, &config);

        if (requestedStart) {
            srand(config.gameSettings.seed);
            startGameHandling(config);
        }
    }
}
