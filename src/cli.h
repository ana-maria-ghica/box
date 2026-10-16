#ifndef CLI_H
#define CLI_H

struct cli_args {
    char* command;
    char* rootfs;
    char* hostname;
    char **programs;
    long pid_max;
    long mem_max;
    double cpu_max;
    char* ip;
    char* name;
};

void parse_cli(char** argv, struct cli_args* args);

#endif