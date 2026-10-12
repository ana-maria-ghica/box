#define _GNU_SOURCE

#include "cgroups.h"
#include "util.h"
#include "cli.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define PATH_SIZE 256

void write_file(char* path, char* mesg) {
    int ret;

    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror(path);
        exit(1);
    }

    ret = write(fd, mesg, strlen(mesg));
    if (ret < 0) {
        perror("write");
        exit(1);
    }

    close(fd);
}

void cgroups_init(int pid, struct cli_args* args) {
    int ret;

    // creez directorul
    char path[PATH_SIZE];
    ret = snprintf(path, sizeof(path), "/sys/fs/cgroup/box-%d", pid);
    if (ret < 0 || (size_t)ret >= sizeof(path)) {
        perror("snprintf");
        exit(1);
    }

    ret = mkdir(path, 0777);
    CHECK_IGNORE_EEXIST(ret, "mkdir");

    // activez controllerele
    write_file("/sys/fs/cgroup/cgroup.subtree_control", "+pids +memory +cpu\n");

    // scriu limitele
    char value[32];

    if (args->pid_max > 0) {
        char pid_limit_path[PATH_SIZE];
        snprintf(pid_limit_path, sizeof(pid_limit_path), "/sys/fs/cgroup/box-%d/pids.max", pid);
        snprintf(value, sizeof(value), "%ld", args->pid_max);
        write_file(pid_limit_path, value);
    }

    if (args->mem_max > 0) {
        char mem_limit_path[PATH_SIZE];
        snprintf(mem_limit_path, sizeof(mem_limit_path), "/sys/fs/cgroup/box-%d/memory.max", pid);
        snprintf(value, sizeof(value), "%ld", args->mem_max);
        write_file(mem_limit_path, value);
    }

    if (args->cpu_max > 0) {
        char cpu_limit_path[PATH_SIZE];
        snprintf(cpu_limit_path, sizeof(cpu_limit_path), "/sys/fs/cgroup/box-%d/cpu.max", pid);

        long quota = (long)(args->cpu_max * 100000);  // de ex: 0.5 × 100000 = 50000
        snprintf(value, sizeof(value), "%ld 100000", quota);  // de ex: "50000 100000"
        write_file(cpu_limit_path, value);
    }


    // scriu PID-ul copilului in cgroups.procs
    char procs_path[PATH_SIZE];
    snprintf(procs_path, sizeof(procs_path), "/sys/fs/cgroup/box-%d/cgroup.procs", pid);
    snprintf(value, sizeof(value), "%d", pid);
    write_file(procs_path, value);
}
