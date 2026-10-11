#define _GNU_SOURCE
#include "child.h"
#include "cli.h"
#include "util.h"
#include "container.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int child_function(void* arg) {
    int ret;

    struct intern_struct* s = (struct intern_struct*) arg; 
    struct cli_args* args = s->args;
    int* pipe_fd = s->pipe;
    close(pipe_fd[1]);

    // procesul copil asteapta sincronizarea
    char buf[2];
    ret = read(pipe_fd[0], &buf, sizeof(char));
    CHECK(ret);

    close(pipe_fd[0]);

    ret = sethostname(args->hostname, strlen(args->hostname));
    CHECK(ret);

    execve(args->programs[0], args->programs, NULL);
    perror("execve");
    _exit(127);

    return 0;
}