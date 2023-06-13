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
#include <unistd.h>

#include "common.h"

int jhandler_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;
	list = loptend;
	if(list->next)
		return EX_USAGE;

	if(list)
	{
		if(is_handler(list->word->word))
			return EXECUTION_SUCCESS;
	}
	else if(!isatty(fileno(stdin)))
	{
		if(get_handler_stdin() >= 0)
			return EXECUTION_SUCCESS;
	}
	else
		return EX_USAGE;
	return EXECUTION_FAILURE;
}

char *jhandler_doc[] = {
	"jhandler <handler>",
	"",
	"test if a string is a handler",
	NULL
};

struct builtin jhandler_struct = {
	"jhandler",
	jhandler_builtin,
	BUILTIN_ENABLED,
	jhandler_doc,
	"jhandler <handler>",
	0
};
