#ifndef NETWORK_H
#define NETWORK_H

#include <unistd.h>
#include <sys/types.h>
#include "cli.h"

void run_command(char** args);
void setup_network_host(pid_t pid);
void setup_network_container(struct cli_args* args);

#endif