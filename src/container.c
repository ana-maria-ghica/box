#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "util.h"
#include "cli.h"
#include "container.h"

int run_container(struct cli_args* args) {
    pid_t pid = fork();
    CHECK(pid);

    // copil
    if (pid == 0) {
        execve(args->programs[0], args->programs, NULL);
        perror("execve");
        _exit(127);
    }

    // parinte
    int status;
    int res = waitpid(pid, &status, 0);
    CHECK(res);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return 1;   
}