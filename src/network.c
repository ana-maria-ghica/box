#include "network.h"
#include "util.h"
#include "cli.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void run_command(char** args) {
    // fprintf(stderr, "running: %s %s %s...\n", args[0], args[1] ? args[1] : "", args[2] ? args[2] : "");
    int ret;
    pid_t pid = fork();

    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        _exit(127);
    }

    int status;
    ret = waitpid(pid, &status, 0);
    CHECK(ret);

    if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0)){
        fprintf(stderr, "Command failed: %s\n", args[0]);
        exit(1);
    }
}

// setez network parinte
void setup_network_host(pid_t pid) {
    // creez virtual eth pair
    char *create_veth[] = {"ip", "link", "add", "veth-h", "type", "veth", "peer", "name", "veth-c", NULL};
    run_command(create_veth);

    // pun veth-c in namespace-ul copilului
    char pid_string[16];
    snprintf(pid_string, sizeof(pid_string), "%d", pid);
    char* move_veth_c[] = {"ip", "link", "set", "veth-c", "netns", pid_string, NULL};
    run_command(move_veth_c);

    // setez interfata de loopback UP
    char *loopback_up[] = {"ip", "link", "set", "lo", "up", NULL};
    run_command(loopback_up);

   // setez adresa ip pe veth-h
    char *ip_veth_h[] = {"ip", "addr", "add", "10.0.0.1/24", "dev", "veth-h", NULL};
    run_command(ip_veth_h);

    // setez interfata veth-h UP
    char *set_ip_up[] = {"ip", "link", "set", "veth-h", "up", NULL};
    run_command(set_ip_up);
}

// setez network copil
void setup_network_container(struct cli_args* args) {
   // setez adresa ip pe veth-c
    char *ip_veth_c[] = {"ip", "addr", "add", args->ip, "dev", "veth-c", NULL};
    run_command(ip_veth_c);

    // setez interfata veth-c UP
    char *set_ip_up[] = {"ip", "link", "set", "veth-c", "up", NULL};
    run_command(set_ip_up);

    char *route[] = {"ip", "route", "add", "default", "via", "10.0.0.1", NULL};
    run_command(route);
}
