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
#include <errno.h>

#include "common.h"

int jload_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;
	list = loptend;
	if(list)
		if(list->next)
			// 2+ arguments
			return EX_USAGE;

	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		PE("failed to open shared memory");
		return EXECUTION_FAILURE;
	}

	FILE *target = stdin;	// default
	if(list)
	{
		if((target = fopen(list->word->word, "r"))==NULL)
		{
			PE("failed to open file: %s", strerror(errno));
			return EXECUTION_FAILURE;
		}
	}

	long object = j_parse_file(shm, target, 0);
	if(object<0)
	{
		shmem_fini(shm);
		PE("failed to load JSON");
		return EXECUTION_FAILURE;
	}

	if(target!=stdin)
		fclose(target);

	print_handler(shm, object);

	shmem_fini(shm);

	return EXECUTION_SUCCESS;
}

char *jload_doc[] = {
	"jload builtin",
	"",
	"loads a JSON object from STDIN",
	"returns a JSON handler.",
	NULL
};

struct builtin jload_struct = {
	"jload",
	jload_builtin,
	BUILTIN_ENABLED,
	jload_doc,
	"jload",
	0
};
