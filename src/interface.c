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
        .distribution = UNIFORM,
        .seed = time(NULL),
        .timeLimit = 5.0,
        .depth = 0,
        .startColor = 1,
        .autoplay = true
    };

    // Global loop
    bool requestedQuit = false;
    while (!requestedQuit) {
        bool requestedStart = false;
        handleConfigInput(&requestedQuit, &requestedStart, &config);

        if (requestedStart) {
            startGameHandling(&config);
        }
    }
}
