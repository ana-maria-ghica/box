#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "cli.h"
#include "container.h"
#include "util.h"

int main(int argc, char**argv) {
    (void)argc;

    // definesc structura cu argumente
    struct cli_args args = {0};
    
    // parsez inputul din cli
    parse_cli(argv, &args);

    // sanity check - command
    if (args.command == NULL) {
        fprintf(stderr, "Usage: ./box <run|ps|exec|stop> ...\n");
        return 1;
    }

    // sanity check pentru comenzi
    if (strcmp(args.command, "run") == 0) {
        if (args.rootfs == NULL || args.name == NULL || args.programs == NULL || args.programs[0] == NULL) {
            fprintf(stderr, "run: --rootfs and a program are required\n");
            exit(1);
        }
        // creez containerul
        return run_container(&args);

    } else if (strcmp(args.command, "exec") == 0) {
        if (args.name == NULL || args.programs == NULL || args.programs[0] == NULL) {
            fprintf(stderr, "exec: --name and a program are required\n");
            exit(1);
        }
        return exec_container(&args);

    } else if (strcmp(args.command, "ps") == 0) {
        return ps_container();

    } else if (strcmp(args.command, "stop") == 0) {
        if (args.name == NULL) {
            fprintf(stderr, "stop: --name is required\n");
            exit(1);
        }
        return stop_container(&args);

    } else {
        fprintf(stderr, "unknown command: %s\n", args.command);
        return 1;
    }
}