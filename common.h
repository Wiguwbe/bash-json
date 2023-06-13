
#ifndef _BASH_JSON_COMMON_H_
#define _BASH_JSON_COMMON_H_ 1

#include <stdio.h>
#include <unistd.h>

#include <config.h>
#include <builtins.h>
#include <shell.h>
#include <bashgetopt.h>
#include <common.h>

#include "json.h"
#include "shmalloc.h"

#ifdef PD
#undef PD
#endif
#ifdef DEBUG
#define PD(fmt, ...) fprintf(stderr, fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#else
#define PD(fmt, ...)
#endif

#ifdef PE
#undef PE
#endif
#define PE(fmt, ...) fprintf(stderr, "error: " fmt "\n" __VA_OPT__(,) __VA_ARGS__)

extern char shm_name[16];

int init_top_level(void);
void fini_top_level(void);

// 1 if is handler
int is_handler(char *s);
// get handler, fail if is not
long get_handler(char *s);
long get_handler_stdin(void);

char *read_stdin_all(int *len_out);

// will print handler
// wither a j:xx for complex types, or the value from the object for simple types
void print_handler(void *shm, long obj);

#endif
