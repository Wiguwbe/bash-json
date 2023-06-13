
#include "common.h"

static char *_type_map[] = {
	// JTYPE_NULL
	"null",
	"dict",
	"list",
	"int",
	"float",
	"string",
	"bool",
	"bool"
};

int jtype_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;

	list = loptend;
	if(list)
		if(list->next)
			return EX_USAGE;

	long obj = -1;

	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		PE("failed to open shared memory");
		return EXECUTION_FAILURE;
	}

	if(list)
	{
		char *handler_str = list->word->word;
		if(!is_handler(handler_str))
		{
			shmem_fini(shm);
			PE("handler is not actually a handler");
			return EX_USAGE;
		}
		obj = get_handler(handler_str);
	}
	else if(!isatty(fileno(stdin)))
		obj = get_handler_stdin();
	else
	{
		shmem_fini(shm);
		return EX_USAGE;
	}

	PD("obj is %ld", obj);

	if(obj<0)
	{
		PE("invalid handler");
		shmem_fini(shm);
		return EXECUTION_FAILURE;
	}

	printf("%s\n", _type_map[j_type(shm, obj)]);

	shmem_fini(shm);

	return EXECUTION_SUCCESS;
}

// no load/unload

char *jtype_doc[] = {
	"jtype <handler>",
	"",
	"prints the type of the object",
	NULL
};

struct builtin jtype_struct = {
	"jtype",
	jtype_builtin,
	BUILTIN_ENABLED,
	jtype_doc,
	"jtype <handler>",
	0
};
