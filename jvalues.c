
#include "common.h"

static int _iter_dict(void *shm, char *key, long value, void *ud)
{
	print_handler(shm, value);
	return 0;
}

static int _iter_list(void *shm, int index, long value, void *ud)
{
	print_handler(shm, value);
	return 0;
}

int jvalues_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;

	list = loptend;
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
		goto _usage;

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
	case JTYPE_LIST:
		j_list_iter(shm, obj, _iter_list, NULL);
		break;
	default:
		PE("`jvalues` only works for dict/list objects");
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

char *jvalues_doc[] = {
	"jvalues <handler>",
	"",
	"print a (bash) array of the objects the collection has",
	NULL
};

struct builtin jvalues_struct = {
	"jvalues",
	jvalues_builtin,
	BUILTIN_ENABLED,
	jvalues_doc,
	"jvalues <handler>",
	0
};
