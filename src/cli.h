#ifndef CLI_H
#define CLI_H

struct cli_args {
    char* rootfs;
    char* hostname;
    char **programs;
};

void parse_cli(char** argv, struct cli_args* args);

#endif