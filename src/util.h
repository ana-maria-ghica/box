#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(call_result)                                                                           \
    do {                                                                                             \
        if (call_result == -1) {                                                                     \
            fprintf(stderr, "%s, %d: %s - %s\n", __FILE__, __LINE__, #call_result, strerror(errno)); \
            exit(1);                                                                                 \
        }                                                                                            \
    } while(0)

#endif