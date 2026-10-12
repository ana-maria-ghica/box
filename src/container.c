#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sched.h>

#include "util.h"
#include "cli.h"
#include "container.h"
#include "child.h"
#include "cgroups.h"

#define CHILD_STACK_SIZE 65536

int run_container(struct cli_args* args) {
    int ret;

    // pipe-ul de sincronizare
    int pipe_fd[2];
    ret = pipe(pipe_fd);
    CHECK(ret);

    // aloc stack-ul pentru copil
    char* child_stack = malloc(sizeof(char) * CHILD_STACK_SIZE);
    if (!child_stack) {
        perror("malloc");
        return 1;
    }

    // stack-ul creste in jos => merg cu pointerul la final
    char* stackTop = child_stack + CHILD_STACK_SIZE;

    // creez structura interna ca sa trimit args si pipe
    struct intern_struct s;
    s.args = args;
    s.pipe = pipe_fd;

    pid_t pid = clone(child_function, stackTop, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNS | SIGCHLD, &s);
    CHECK(pid);
    close(pipe_fd[0]);  // inchid capatul de citire

    // implementarea cgroups - pana sa trimita byte-ul prin pipe
    cgroups_init(pid, args);

    // parintele scrie in pipe
    ret = write(pipe_fd[1], "a", sizeof(char));
    CHECK(ret);
    close(pipe_fd[1]);

    int status;
    ret = waitpid(pid, &status, 0);
    CHECK(ret);

    // sterg directoarele create de cgroup
    char cgroup_path_remove[256];
    snprintf(cgroup_path_remove, sizeof(cgroup_path_remove), "/sys/fs/cgroup/box-%d", pid);
    ret = rmdir(cgroup_path_remove);
    if (ret == -1)
        perror("rmdir");
        
    // eliberez stiva
    free(child_stack);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return 1;
}



// ------------------------------------------------------------------------------------------------------------------------
/*
    Initial implementation using fork()

    pid_t pid = fork();
    CHECK(pid);
    // copil
    if (pid == 0) {
        execve(args->programs[0], args->programs, NULL);  // -> trebuie sa pun argumentele intr-o structura si sa le pasez ca (void*)& in apelul de clone()
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
*/ 