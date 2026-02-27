/**
 * Copyright (c) Alexander Kurtz 2026
*/

#include "user/interface.h"

#ifndef WEB_BUILD
#include "user/runBenchmark.h"
#endif

/**
 * Program entry point
*/
int main(int argc, char const* argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--benchmark") == 0) {
#ifndef WEB_BUILD
            runBenchmark();
#endif
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
