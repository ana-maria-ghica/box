#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "cli.h"
#include "container.h"
#include "util.h"

int main(int argc, char**argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage: ./box run --rootfs <directory> --hostname <name> <program> [args...]\n");
        return 1;
    }

    struct cli_args args = {0};
    
    // parsez inputul din cli
    parse_cli(argv, &args);
    if (args.rootfs == NULL || args.hostname == NULL)
        return 1;

    // creez containerul
    return run_container(&args);
}