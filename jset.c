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

int jset_builtin(WORD_LIST *list)
{
	/*
	// the index, to append, may be -1, which may be confused with
	// an option
	if(no_options(list))
		return EX_USAGE;
	*/

	PD("list %p", list);

	//list = loptend;
	if(!list)	 return EX_USAGE;
	if(!list->next)
		// at least 2 arguments
		return EX_USAGE;
	PD("next %p", list->next);
	// but no more than 3
	if(list->next->next)
		if(list->next->next->next)
			// 4+ arguments
			return EX_USAGE;

	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		PE("failed to open shared memory");
		return EXECUTION_FAILURE;
	}

	long obj;
	// either one of these
	int index;
	char *key;
	long value = -1;
	int obj_type;
	if(list->next->next)
	{
		// 3 arguments, all from CLI
		char *obj_handler = list->word->word;
		if(!is_handler(obj_handler))
		{
			PE("invalid handler");
			goto _usage;
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
		goto _usage;
	}

	if(obj < 0)
	{
		PE("invalid object handler");
		goto _fail;
	}

	// key/index
	obj_type = j_type(shm, obj);
	if(obj_type == JTYPE_DICT)
	{
		char *key_input = list->word->word;
		if(is_handler(key_input))
		{
			long key_obj = get_handler(key_input);
			if(key_obj < 0)
			{
				PE("invalid handler for key");
				goto _fail;
			}
			if(j_type(shm, key_obj) != JTYPE_STR)
			{
				PE("key is not a string");
				goto _fail;
			}
			key = j_str_val(shm, key_obj);
		}
		else
			key = key_input;
		PD("key is %s", key);
	}
	else if(obj_type == JTYPE_LIST)
	{
		char *index_input = list->word->word;
		if(is_handler(index_input))
		{
			long index_obj = get_handler(index_input);
			if(index_obj < 0)
			{
				PE("invalid handler for index");
				goto _fail;
			}
			if(j_type(shm, index_obj) != JTYPE_INT)
			{
				PE("index is not an integer");
				goto _fail;
			}
			index = (int)j_int_val(shm, index_obj);
		}
		else
		{
			char *end;
			index = (int)strtol(index_input, &end, 10);
			if(*end)
			{
				PE("index is not an integer");
				goto _fail;
			}
		}
		PD("index is %d", index);
	}
	else
	{
		PE("cant `jdel` from non dict/list");
		goto _fail;
	}

	list = list->next;

	// the value to set
	{
		char *value_input = list->word->word;
		// check handler
		if(is_handler(value_input))
		{
			value = get_handler(value_input);
		}
		// check JSON string
		if(value < 0)
		{
			value = j_parse_buffer(shm, value_input, strlen(value_input), 1);
		}
		// fallback to string literal (should catch the other cases on the previous branches
		if(value < 0)
		{
			value = j_str_new(shm, value_input);
		}
	}

	if(value < 0)
	{
		PE("failed to get/create JSON object from input");
		goto _fail;
	}

	{
		int r;
		if(obj_type == JTYPE_DICT)
			r = j_dict_set(shm, obj, key, value);
		else
			r = j_list_set(shm, obj, index, value);
		if(r)
		{
			PE("failed to set");
			goto _fail;
		}
	}

	shmem_fini(shm);

	return EXECUTION_SUCCESS;

_usage:
	shmem_fini(shm);
	return EX_USAGE;
_fail:
	shmem_fini(shm);
	return EXECUTION_FAILURE;
}

// no load/unload

char *jset_doc[] = {
	"jset <handler> <key|index> <JSON|handler>",
	"",
	"Set object in list/dict, to append to a list pass '-1' as index",
	NULL
};

struct builtin jset_struct = {
	"jset",
	jset_builtin,
	BUILTIN_ENABLED,
	jset_doc,
	"jset <handler> <key|index> <JSON|handler>",
	0
};
