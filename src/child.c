#define _GNU_SOURCE
#include "child.h"
#include "cli.h"
#include "util.h"
#include "container.h"
#include "mounts.h"
#include "security.h"
#include "network.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <wait.h>

static pid_t child_pid;

static void forward_signal(int sig) {
    if (child_pid > 0)
        // paseaza semnalul copilului
        kill(child_pid, sig);
}

int child_function(void* arg) {
    int ret;

    struct intern_struct* s = (struct intern_struct*) arg; 
    struct cli_args* args = s->args;
    int* pipe_fd = s->pipe;
    close(pipe_fd[1]);

    // procesul copil asteapta sincronizarea
    char buf[2];
    ret = read(pipe_fd[0], &buf, sizeof(char));
    if (ret <= 0) {
        perror("read");
        exit(1);
    }

    close(pipe_fd[0]);

    // fac mount la root -----------------------------------------
    ret = mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL);
    CHECK(ret);

    // modific rootfs-ul -----------------------------------------
    setup_rootfs();

    // montez /proc
    ret = mount("proc", "/proc", "proc", 0, NULL);
    CHECK(ret);

    // montez /sys
    ret = mount("sysfs", "/sys", "sysfs", MS_RDONLY, NULL);
    CHECK(ret);

    // montez /dev
    ret = mount("tmpfs", "/dev", "tmpfs", 0, NULL);
    CHECK(ret);

    // creez nodurile esentiale in /dev: null, zero, random, tty
    ret = mknod("/dev/null", S_IFCHR | 0666, makedev(1, 3));
    CHECK_IGNORE_EEXIST(ret, "mknod");


    ret = mknod("/dev/zero", S_IFCHR | 0666, makedev(1, 5));
    CHECK_IGNORE_EEXIST(ret, "mknod");

    ret = mknod("/dev/random", S_IFCHR | 0666, makedev(1, 8));
    CHECK_IGNORE_EEXIST(ret, "mknod");


    ret = mknod("/dev/urandom", S_IFCHR | 0666, makedev(1, 9));
    CHECK_IGNORE_EEXIST(ret, "mknod");


    ret = mknod("/dev/tty", S_IFCHR | 0666, makedev(5, 0));
    CHECK_IGNORE_EEXIST(ret, "mknod");


    // setez hostname -----------------------------------------
    ret = sethostname(args->hostname, strlen(args->hostname));
    CHECK(ret);

    // restrictionez capabilitatile ------------------------------
    restrict_process();

    // setez network-ul containerului ------------------------------
    setup_network_container(args);
    
    // execut efectiv programul -----------------------------------------

    // implementez mini-init
    pid_t pid = fork();
    child_pid = pid;

    if (pid == 0) {
        execve(args->programs[0], args->programs, NULL);
        perror("execve");
        _exit(127);
    }

    
    int status;
    pid_t reaped;
    while ((reaped = waitpid(-1, &status, 0)) > 0) {
        // am cules un copil: shell-ul sau un orfan
        if (reaped == pid) {
            // programul real a murit; init se termina
            break;
        }
        // altfel: era un orfan; continui bucla
    }

    // parintele - procesul init
    signal(SIGTERM, forward_signal);
    signal(SIGINT, forward_signal);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return 1;
}