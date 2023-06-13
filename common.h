/*
 * bash-json
 * <http://github.com/Wiguwbe/bash-json>
 *
 * Copyright (c) 2023 Tiago Teixeira
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

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
