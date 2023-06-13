
#include "common.h"

int jlen_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;

	list = loptend;
	if(list)
	{
		if(list->next)
			return EX_USAGE;
	}

	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		fprintf(stderr, "error: failed to open shared memory\n");
		return EXECUTION_FAILURE;
	}

	long obj;
	if(list)
	{
		char *handler_str = list->word->word;
		if(!is_handler(handler_str))
		{
			PE("invalid handler");
			goto _usage;
		}
		obj = get_handler(handler_str);
	}
	else if(!isatty(fileno(stdin)))
	{
		obj = get_handler_stdin();
	}
	else
		goto _usage;

	int len;
	switch(j_type(shm, obj))
	{
	case JTYPE_DICT:
		len = j_dict_len(shm, obj);
		break;
	case JTYPE_LIST:
		len = j_list_len(shm, obj);
		break;
	default:
		PE("`jlist` not valid for non dict/list types");
		goto _fail;
	}

	printf("%d\n", len);

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

char *jlen_doc[] = {
	"jlen <handler>",
	"",
	"get the length of the dict/list",
	NULL
};

struct builtin jlen_struct = {
	"jlen",
	jlen_builtin,
	BUILTIN_ENABLED,
	jlen_doc,
	"jlen <handler>",
	0
};
