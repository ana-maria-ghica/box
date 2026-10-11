#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "cli.h"

/*
struct cli_args {
    char* rootfs;
    char* hostname;
    char **programs;
};
*/

void parse_cli(char** argv, struct cli_args* args) {
    if (strcmp(argv[1], "run") == 0) {
        int counter = 2;
        while(true) {
            if (strcmp(argv[counter], "--rootfs") == 0) {
                args->rootfs = argv[++counter];
                ++counter;

            } else if (strcmp(argv[counter], "--hostname") == 0) {
                args->hostname = argv[++counter];
                ++counter;
            } else {
                break;
            }
        }

        args->programs = &argv[counter];
    } else {
        fprintf(stderr, "Wrong input\n");
        return;
    }
    return;
}