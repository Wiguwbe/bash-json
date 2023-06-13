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

#include "common.h"

int jget_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;

	if(!list)
		return EX_USAGE;

	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		PE("failed to open shared memory");
		return EXECUTION_FAILURE;
	}

	long obj;
	long out;
	// get obj
	if(list->next)
	{
		// 2+ arguments
		// btw
		if(list->next->next)
		{
			// 3+ arguments
			shmem_fini(shm);
			return EX_USAGE;
		}
		char *obj_handler = list->word->word;
		if(!is_handler(obj_handler))
		{
			PE("invalid handler");
			shmem_fini(shm);
			return EX_USAGE;
		}
		obj = get_handler(obj_handler);
		list = list->next;
	}
	else if(!isatty(fileno(stdin)))
	{
		obj = get_handler_stdin();
	}
	else
	{
		shmem_fini(shm);
		return EX_USAGE;
	}

	if(obj<0)
	{
		PE("invalid object handler");
		shmem_fini(shm);
		return EX_USAGE;
	}

	if(j_type(shm, obj) == JTYPE_DICT)
	{
		char *key_input = list->word->word;
		char *key = NULL;
		if(is_handler(key_input))
		{
			// treat as handler
			long obj_key = get_handler(key_input);
			if(obj_key < 0)
			{
				PE("key is invalid JSON handler");
				shmem_fini(shm);
				return EXECUTION_FAILURE;
			}
			if(j_type(shm, obj_key) != JTYPE_STR)
			{
				PE("key is not a string");
				shmem_fini(shm);
				return EXECUTION_FAILURE;
			}
			key = j_str_val(shm, obj_key);
		}
		else
		{
			key = key_input;
		}

		out = j_dict_get(shm, obj, key);
	}
	else if(j_type(shm, obj) == JTYPE_LIST)
	{
		char *index_input = list->word->word;
		int index = -1;
		if(is_handler(index_input))
		{
			long obj_index = get_handler(index_input);
			if(obj_index < 0)
			{
				PE("index is invalid JSON handler");
				shmem_fini(shm);
				return EXECUTION_FAILURE;
			}
			if(j_type(shm, obj_index) != JTYPE_INT)
			{
				PE("index is not an integer");
				shmem_fini(shm);
				return EXECUTION_FAILURE;
			}
			index = (int)j_int_val(shm, obj_index);
		}
		else
		{
			char *end;
			index = (int)strtol(index_input, &end, 10);
			if(*end)
			{
				PE("index is not an integer");
				shmem_fini(shm);
				return EXECUTION_FAILURE;
			}
		}

		out = j_list_get(shm, obj, index);
	}
	else
	{
		PE("can't `jget` from non dict/list objects");
		shmem_fini(shm);
		return EXECUTION_FAILURE;
	}

	if(out < 0)
	{
		PE("not found");
		shmem_fini(shm);
		return EXECUTION_FAILURE;
	}

	print_handler(shm, out);

	shmem_fini(shm);

	return EXECUTION_SUCCESS;
}

// no load/unload

char *jget_doc[] = {
	"jget <handler> <key|index>",
	"",
	"get object from collection",
	NULL
};

struct builtin jget_struct = {
	"jget",
	jget_builtin,
	BUILTIN_ENABLED,
	jget_doc,
	"jget <handler> <key|index>",
	0
};
