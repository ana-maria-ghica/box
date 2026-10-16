#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cli.h"

/*
struct cli_args {
    char* rootfs;
    char* hostname;
    char **programs;
    long pid_max;
    long mem_max;
    double cpu_max;
};
*/

void parse_cli(char** argv, struct cli_args* args) {
    int counter = 2;

    if (strcmp(argv[1], "run") == 0) {
        args->command = "run";

        while(true) {
            if (strcmp(argv[counter], "--rootfs") == 0) {
                args->rootfs = argv[counter + 1];
                counter += 2; 

            } else if (strcmp(argv[counter], "--hostname") == 0) {
                args->hostname = argv[counter + 1];
                counter += 2; 

            } else if (strcmp(argv[counter], "--pids") == 0) {
                args->pid_max = atol(argv[counter + 1]);
                counter += 2; 

            } else if (strcmp(argv[counter], "--mem") == 0) {
                char *val = argv[counter + 1];
                long number = atol(val);

                if (strchr(val, 'G') != NULL) {
                    args->mem_max = number * 1024L * 1024L * 1024L;
                } else if (strchr(val, 'M') != NULL) {
                    args->mem_max = number * 1024L * 1024L;
                } else {
                    args->mem_max = number;
                }
                counter += 2;             

            } else if (strcmp(argv[counter], "--cpu") == 0) {
                args->cpu_max = atof(argv[counter + 1]);
                counter += 2; 

            } else if (strcmp(argv[counter], "--net") == 0) {
                args->ip = argv[counter + 1];
                counter += 2;

            } else if (strcmp(argv[counter], "--name") == 0) {
                args->name = argv[counter + 1];
                counter += 2;
            } else {
                break;
            }
        }
        args->programs = &argv[counter];

    } else if (strcmp(argv[1], "ps") == 0) {
        args->command = "ps";
        // afiseaza dockerele

    } else if (strcmp(argv[1], "exec") == 0) {
        args->command = "exec";
        args->name = argv[counter];
        ++counter;
        args->programs = &argv[counter];
        // da run la un docker
        
    } else if (strcmp(argv[1], "stop") == 0) {
        args->command = "stop";
        args->name = argv[counter];
        ++counter;
        // opreste docker
        
    } else {
        fprintf(stderr, "Wrong input\n");
        return;
    }
    return;
}