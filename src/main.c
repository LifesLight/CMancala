/**
 * Copyright (c) Alexander Kurtz 2023
*/

#include "interface.h"

/**
 * Program entry point
*/
int main(int argc, char const* argv[]) {
    if (argc > 1) {
        printf("CMancala does not support command line arguments. Ignoring:\n[");
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
