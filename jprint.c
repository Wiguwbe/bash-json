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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"

#include "json-parser.h"

void _print_json(void *shm, long object);


static int _write_data(const char *data, size_t size, void *unused)
{
	// use `!=` to return 0 on success
	return fwrite(data, 1, size, stdout) != size;
}

static int _print_dict(void *shm, char *key, long value, void *_ud)
{
	int *l = (int*)_ud;
	json_dump_string(key, strlen(key), _write_data, NULL);
	putchar(':');
	_print_json(shm, value);
	if(*l != 1)
		printf(",");
	(*l)--;
	return 0;
}
static int _print_list(void *shm, int index, long value, void *_ud)
{
	int *l = (int*)_ud;
	_print_json(shm, value);
	if(*l != 1)
		printf(",");
	(*l)--;
	return 0;
}

void _print_json(void *shm, long object)
{
	switch(j_type(shm, object))
	{
	case JTYPE_NULL: printf("null"); break;
	case JTYPE_TRUE: printf("true"); break;
	case JTYPE_FALSE: printf("false"); break;
	case JTYPE_INT: json_dump_int64(j_int_val(shm, object), _write_data, NULL); break;
	case JTYPE_FLOAT: json_dump_double(j_float_val(shm, object), _write_data, NULL); break;
	case JTYPE_STR: {
		char *s = j_str_val(shm, object);
		json_dump_string(s, strlen(s), _write_data, NULL);
		break;
	}
	case JTYPE_DICT: {
		int l = j_dict_len(shm, object);
		printf("{");
		if(j_dict_iter(shm, object, _print_dict, &l))
			return;
		printf("}");
		break;
	}
	case JTYPE_LIST: {
		int l = j_list_len(shm, object);
		printf("[");
		if(j_list_iter(shm, object, _print_list, &l))
			return;
		printf("]");
		break;
	}
	}
	fflush(stdout);
}

int jprint_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;
	list = loptend;
	if(list&&list->next)
		// only one argument
		return EX_USAGE;

	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		PE("failed to open shared memory");
		return EXECUTION_FAILURE;
	}

	long ptr_object = -1;
	if(list)
	{
		char *handler = list->word->word;
		if(!is_handler(handler))
		{
			shmem_fini(shm);
			PE("invalid handler");
			return EXECUTION_FAILURE;
		}
		ptr_object = get_handler(handler);
	}
	else if(!isatty(fileno(stdin)))
		ptr_object = get_handler_stdin();
	else
		return EX_USAGE;

	if(ptr_object<0)
	{
		shmem_fini(shm);
		PE("invalid handler");
		return EXECUTION_FAILURE;
	}
	// output
	_print_json(shm, ptr_object);
	putchar(10);

	shmem_fini(shm);

	return EXECUTION_SUCCESS;
}

char *jprint_doc[] = {
	"jprint <handler>",
	"",
	"prints the JSON representation of the JSON object specified by the handler.",
	NULL
};

struct builtin jprint_struct = {
	"jprint",
	jprint_builtin,
	BUILTIN_ENABLED,
	jprint_doc,
	"jprint <handler>",
	0
};
