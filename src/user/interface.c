/**
 * Copyright (c) Alexander Kurtz 2026
 */

#include "user/interface.h"
#include "logic/solver/egdb/core.h"
#include "logic/solver/cache.h"
#include "logic/utility.h"

void runApiMode(int argc, char const* argv[]) {
    SolverConfig config = {
        .solver = LOCAL_SOLVER,
        .depth = 0,
        .timeLimit = 0.0,
        .clip = false,
        .compressCache = AUTO,
        .progressBar = false,
        .useOpeningBook = false
    };

    int egdb_stones = 0;
    bool avalanche = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--egdb") == 0 && i + 1 < argc) {
            egdb_stones = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--cache") == 0 && i + 1 < argc) {
            setCacheSize(atoi(argv[++i]));
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (strcmp(argv[++i], "avalanche") == 0) avalanche = true;
        }
    }

    if (avalanche) setMoveFunction(AVALANCHE_MOVE);
    else setMoveFunction(CLASSIC_MOVE);

    if (egdb_stones > 0) {
        generateEGDB(egdb_stones, avalanche);
    }

    char line[1024];
    while (fgets(line, sizeof(line), stdin)) {
        trimSpaces(line);
        if (strlen(line) == 0) continue;

        if (strcmp(line, "QUIT") == 0) {
            break;
        }

        if (strncmp(line, "SOLVE ", 6) == 0) {
            char* rawCode = line + 6;
            char code[256];
            // Encode stripped 5 zeros dynamically 
            snprintf(code, sizeof(code), "00000%.*s", (int)(sizeof(code) - 6), rawCode);

            Board board;
            if (!decodeBoard(&board, code)) {
                printf("ERROR invalid_code\n");
                fflush(stdout);
                continue;
            }

            // Set correct stone count for EGDB
            int totalStones = 0;
            for (int i = 0; i < 14; i++) totalStones += board.cells[i];
            setStoneCount(totalStones);

            Context ctx;
            memset(&ctx, 0, sizeof(Context));
            ctx.metadata.lastMove = -1;
            ctx.metadata.lastEvaluation = INT32_MAX;
            Board lastBoard;
            memset(&lastBoard, 0, sizeof(Board));
            ctx.board = &board;
            ctx.lastBoard = &lastBoard;

            aspirationRoot(&ctx, &config);

            int bestMove = ctx.metadata.lastMove;
            int eval = ctx.metadata.lastEvaluation;

            if (bestMove != -1) {
                makeMoveManual(&board, bestMove);
            }
            char* nextCode = encodeBoard(&board);
            printf("OK EVAL %d MOVE %d NEXT_CODE %s TURN %d\n", eval, bestMove, nextCode, board.color);
            free(nextCode);
            fflush(stdout);

        } else if (strncmp(line, "MOVES ", 6) == 0) {
            char* rawCode = line + 6;
            char code[256];
            snprintf(code, sizeof(code), "00000%.*s", (int)(sizeof(code) - 6), rawCode);

            Board board;
            if (!decodeBoard(&board, code)) {
                printf("ERROR invalid_code\n");
                fflush(stdout);
                continue;
            }

            printf("OK");
            int start = (board.color == 1) ? HBOUND_P1 : HBOUND_P2;
            int end = (board.color == 1) ? LBOUND_P1 : LBOUND_P2;
            for (int i = start; i >= end; i--) {
                if (board.cells[i] > 0) {
                    Board copy = board;
                    makeMoveManual(&copy, i);
                    char* nextCode = encodeBoard(&copy);
                    printf(" MOVE %d NEXT_CODE %s", i, nextCode);
                    free(nextCode);
                }
            }
            printf("\n");
            fflush(stdout);

        } else if (strncmp(line, "ROOT ", 5) == 0) {
            int stones = atoi(line + 5);
            setStoneCount(stones * 12);
            Board board;
            configBoard(&board, stones);
            char* code = encodeBoard(&board);
            printf("OK CODE %s\n", code);
            free(code);
            fflush(stdout);

        } else {
            printf("ERROR unknown_command\n");
            fflush(stdout);
        }
    }
}

void startInterface() {
    renderWelcome();
    setMoveFunction(CLASSIC_MOVE);

    SolverConfig solverConfig = {
        .solver = LOCAL_SOLVER,
        .depth = 0,
        .timeLimit = 5.0,
        .clip = false,
        .compressCache = AUTO,
        .progressBar = true,
        .useOpeningBook = false
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
