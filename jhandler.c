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
