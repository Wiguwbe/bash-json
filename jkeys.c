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

static int _iter_dict(void *shm, char *key, long value, void *ud)
{
	printf("%s\n", key);
	return 0;
}

int jkeys_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;

	if(list)
		if(list->next)
			return EX_USAGE;

	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		fprintf(stderr, "error: failed to open shared memory\n");
		return EXECUTION_FAILURE;
	}

	long obj = -1;

	if(list)
	{
		char *obj_str = list->word->word;
		if(!is_handler(obj_str))
		{
			PE("invalid handler");
			goto _usage;
		}
		obj = get_handler(obj_str);
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
		PE("invalid handler");
		goto _usage;
	}

	switch(j_type(shm, obj))
	{
	case JTYPE_DICT:
		j_dict_iter(shm, obj, _iter_dict, NULL);
		break;
	case JTYPE_LIST: {
		int len = j_list_len(shm, obj);
		for(int i=0;i<len;i++)
			printf("%d\n", i);
		break;
	}
	default:
		PE("`jkeys` is invalid for non dict/list objects");
		goto _fail;
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

char *jkeys_doc[] = {
	"jkeys <handler>",
	"",
	"returns a (bash) array of the keys, or indexes",
	NULL
};

struct builtin jkeys_struct = {
	"jkeys",
	jkeys_builtin,
	BUILTIN_ENABLED,
	jkeys_doc,
	"jkeys <handler>",
	0
};
