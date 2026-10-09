#include <string.h>
#include "cli.h"

void parse_cli(char** argv, struct cli_args* args) {
    if (strcmp(argv[1], "run") == 0) {
        args->rootfs = argv[3];
        args->programs = &argv[4]; 
    }
}