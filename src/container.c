#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <sched.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sched.h>

#include "util.h"
#include "cli.h"
#include "container.h"
#include "child.h"
#include "cgroups.h"
#include "network.h"

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

    pid_t pid = clone(child_function, stackTop, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWNET | SIGCHLD, &s);
    CHECK(pid);
    close(pipe_fd[0]);  // inchid capatul de citire

    // implementarea cgroups - pana sa trimita byte-ul prin pipe
    cgroups_init(pid, args);

    // setez network-ul hostului
    setup_network_host(pid);

    // creez fisierul de stare
    ret = mkdir("/tmp/box", 0777);
    CHECK_IGNORE_EEXIST(ret, "mkdir");

    char state_file_name[128];
    snprintf(state_file_name, sizeof(state_file_name), "/tmp/box/%s", args->name);
    FILE* state_file = fopen(state_file_name, "w");
    if (state_file == NULL) {
        fprintf(stderr, "state_file failed\n");
        exit(1);
    }
    fprintf(state_file, "%d\n", pid);
    fclose(state_file);

    // parintele scrie in pipe
    ret = write(pipe_fd[1], "a", sizeof(char));
    CHECK(ret);
    close(pipe_fd[1]);

    int status;
    ret = waitpid(pid, &status, 0);
    CHECK(ret);

    // sterg state_file
    unlink(state_file_name);

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

// scriu si functiile pentru ps, exec, stop
int ps_container() {
    DIR* dir = opendir("/tmp/box");
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    struct dirent* entry;
    while((entry = readdir(dir)) != NULL) {
        // trec peste intrarile de tipul "." sau ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char path[512];
        snprintf(path, sizeof(path), "/tmp/box/%s", entry->d_name);

        FILE* file = fopen(path, "r");
        if (file == NULL) {
            perror("fopen");
            return 1;
        }

        int pid;
        fscanf(file, "%d", &pid);

        printf("Name: %s; Pid: %d\n", entry->d_name, pid);

        fclose(file);
    }

    closedir(dir);
    return 0;
}

int exec_container(struct cli_args* args) {
    int ret;

    // citesc pid-ul containerului
    char path[512];
    snprintf(path, sizeof(path), "/tmp/box/%s", args->name);

    FILE* file = fopen(path, "r");
    int container_pid;
    fscanf(file, "%d", &container_pid);
    fclose(file);

    // pentru fiecare namespace - il deschid + il setez
    int fd; 

    char pid_path[128];
    snprintf(pid_path, sizeof(pid_path), "/proc/%d/ns/pid", container_pid);
    fd = open(pid_path, O_RDONLY);
    CHECK(fd);
    ret = setns(fd, 0);
    CHECK(ret);
    close(fd);   

    char uts_path[128];
    snprintf(uts_path, sizeof(uts_path), "/proc/%d/ns/uts", container_pid);
    fd = open(uts_path, O_RDONLY);
    CHECK(fd);
    ret = setns(fd, 0);
    CHECK(ret);
    close(fd);   

    char ipc_path[128];
    snprintf(ipc_path, sizeof(ipc_path), "/proc/%d/ns/ipc", container_pid);
    fd = open(ipc_path, O_RDONLY);
    CHECK(fd);
    ret = setns(fd, 0);
    CHECK(ret);
    close(fd);   

    char net_path[128];
    snprintf(net_path, sizeof(net_path), "/proc/%d/ns/net", container_pid);
    fd = open(net_path, O_RDONLY);
    CHECK(fd);
    ret = setns(fd, 0);
    CHECK(ret);
    close(fd);   

    char mnt_path[128];
    snprintf(mnt_path, sizeof(mnt_path), "/proc/%d/ns/mnt", container_pid);
    fd = open(mnt_path, O_RDONLY);
    CHECK(fd);
    ret = setns(fd, 0);
    CHECK(ret);
    close(fd); 

    pid_t pid = fork();
    // procesul copil
    if (pid == 0) {
        execve(args->programs[0], args->programs, NULL);
        perror("execve");
        _exit(127);
    }

    // procesul parinte
    int status;
    ret = waitpid(pid, &status, 0);
    CHECK(ret);

    if (WIFEXITED(status))
        return (WEXITSTATUS(status));

    return 1;
}

int stop_container(struct cli_args* args) {
    // citesc pid-ul containerului
    char path[512];
    snprintf(path, sizeof(path), "/tmp/box/%s", args->name);

    FILE* file = fopen(path, "r");
    int container_pid;
    fscanf(file, "%d", &container_pid);
    fclose(file);

    // trimit SIGTERM
    if (kill(container_pid, SIGTERM) == -1) {
        perror("kill");
        return 1;
    }

    // astept sa se opreasca
    sleep(1);

    // daca inca traieste, SIGKILL (fortat)
    if (kill(container_pid, 0) == 0) {
        kill(container_pid, SIGKILL);
    }

    unlink(path);

    printf("Stopped container %s\n", args->name);
    return 0;
}