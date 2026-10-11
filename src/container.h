#ifndef CONTAINER_H
#define CONTAINER_H

struct intern_struct {
    struct cli_args* args;
    int* pipe;
};

int run_container(struct cli_args *args);

#endif