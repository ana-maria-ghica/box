#ifndef CGROUPS_H
#define CGROUPS_H

#include "cli.h"
void write_file(char* path, char* mesg);
void cgroups_init(int pid, struct cli_args* args);

#endif