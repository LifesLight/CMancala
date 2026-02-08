/**
 * Copyright (c) Alexander Kurtz 2026
*/

#include "user/interface.h"
#include "user/runBenchmark.h"

/**
 * Program entry point
*/
int main(int argc, char const* argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--benchmark") == 0) {
            runBenchmark();
            return 0;
        }

        printf("CMancala does not support command line arguments (except --benchmark). Ignoring:\n[");
        for (int i = 1; i < argc; i++) {
            printf("%s", argv[i]);
            if (i < argc - 1) {
                printf(", ");
            }
        }
        printf("]\n");
    }

    startInterface();
    return 0;
}
