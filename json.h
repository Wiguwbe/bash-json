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
#ifndef _BASH_JSON_JSON_H_
#define _BASH_JSON_JSON_H_

#define JTYPE_NULL	0
#define JTYPE_DICT	1
#define JTYPE_LIST	2
#define JTYPE_INT	3
#define JTYPE_FLOAT	4
#define JTYPE_STR	5
#define JTYPE_TRUE	6
#define JTYPE_FALSE	7

/*
	TODO implement REF COUNTING
*/

// return -1 on error and prints the erron on STDERR
long j_parse_buffer(void *shm, char *buffer, int len, int suppress_errors);
long j_parse_file(void *shm, FILE *file, int suppress_errors);

long j_null_new(void *);
long j_bool_new(void *, int);
long j_int_new(void *, long);
long j_float_new(void *, double);
long j_str_new(void *, char *);
long j_list_new(void *);
long j_dict_new(void *);

void j_free(void *, long);	// generic one
void j_null_free(void *, long);	// actually useless
void j_bool_free(void *, long);	// actually useless
void j_int_free(void *, long);
void j_float_free(void *, long);
void j_str_free(void *, long);
void j_list_free(void *, long);
void j_dict_free(void *, long);

int j_type(void *, long);

long j_int_val(void *, long);
double j_float_val(void *, long);
char *j_str_val(void *, long);	// returns the pointer to shared memory, volatile

long j_list_get(void *, long, int index);
long j_dict_get(void *, long, char *key);

int j_list_set(void *, long, int index, long value);	// works like update (at index) or append (index == -1)
int j_dict_set(void *, long, char *key, long value);

int j_list_del(void *, long, int index);
int j_dict_del(void *, long, char *key);

int j_list_len(void *, long);
int j_dict_len(void *, long);

// iterate functions, callback shall return !0 if wishes to stop the iteration
int j_list_iter(void *, long, int (*callback)(void *shm, int index, long value, void *user_data), void *user_data);
int j_dict_iter(void *, long, int (*callback)(void *shm, char *key, long value, void *user_data), void *user_data);

// compare functions return 0 if equal, !0 otherwise
int j_cmp(void *, long, long);
// these are mostly internal
int j_dict_cmp(void *, long, long);
int j_list_cmp(void *, long, long);

// `has` functions return 1 if TRUE, 0 otherwise
inline int j_dict_haskey(void *shm, long obj, char *key)
{
	return j_dict_get(shm, obj, key) >= 0;
}
int j_list_hasvalue(void *shm, long obj, long search);
int j_dict_hasvalue(void *shm, long obj, long search);

#endif
