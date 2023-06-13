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

#include "common.h"

int jnew_builtin(WORD_LIST *list)
{
	int type = 0, opt;

	reset_internal_getopt();
	while((opt = internal_getopt(list, "dl")) != -1)
	{
		switch(opt)
		{
			case 'd':
				type |= 1;
				break;
			case 'l':
				type |= 2;
				break;
			CASE_HELPOPT;
			default:
				builtin_usage();
				return EX_USAGE;
		}
	}
	list = loptend;
	if(list)
	{
		builtin_usage();
		return EX_USAGE;
	}
	if(!type||type == 3)
	{
		builtin_usage();
		return EX_USAGE;
	}

	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		PE("failed to open shared memory");
		return EXECUTION_FAILURE;
	}

	long obj = type == 1 ? j_dict_new(shm) : j_list_new(shm);
	shmem_fini(shm);
	if(obj<0)
	{
		PE("failed to create object");
		return EXECUTION_FAILURE;
	}
	printf("j:%ld\n", obj);

	return EXECUTION_SUCCESS;
}

int jnew_builtin_load(char *s)
{
	if(init_top_level())
		return 0;
	return 1;
}

void jnew_builtin_unload(char *s)
{
	fini_top_level();
}

char *jnew_doc[] = {
	"jnew <-d|-l>",
	"",
	"create a new dict/list (object/array) JSON handler",
	"returns the new handler",
	NULL
};

struct builtin jnew_struct = {
	"jnew",
	jnew_builtin,
	BUILTIN_ENABLED,
	jnew_doc,
	"jnew <-d|-l>",
	0
};
