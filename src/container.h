#ifndef CONTAINER_H
#define CONTAINER_H

struct intern_struct {
    struct cli_args* args;
    int* pipe;
};

int run_container(struct cli_args *args);
int ps_container();
int exec_container(struct cli_args* args);
int stop_container(struct cli_args* args);

#endif