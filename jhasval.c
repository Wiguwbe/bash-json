
#include "common.h"

int jhasval_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;
	list = loptend;

	if(!list)
		return EX_USAGE;

	if(list->next)
		if(list->next->next)
			return EX_USAGE;

	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		fprintf(stderr, "error: failed to open shared memory\n");
		return EXECUTION_FAILURE;
	}

	long obj = -1;
	long val = -1;
	int free_val = 0;
	int result = 0;

	if(list->next)
	{
		// 2 CLI arguments
		char *handler_str = list->word->word;
		if(!is_handler(handler_str))
		{
			PE("invalid handler");
			goto _usage;
		}
		obj = get_handler(handler_str);
		list = list->next;
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

	// val
	{
		char *obj_str = list->word->word;
		if(is_handler(obj_str))
		{
			val = get_handler(obj_str);
		}
		if(val < 0)
		{
			// try JSON literal
			val = j_parse_buffer(shm, obj_str, strlen(obj_str), 1);
			free_val = 1;
		}
		if(val < 0)
		{
			// fallback to string
			val = j_str_new(shm, obj_str);
			free_val = 1;
		}
		if(val < 0)
		{
			PE("unable to get JSON value");
			goto _fail;
		}
	}

	switch(j_type(shm, obj))
	{
	case JTYPE_DICT:
		result = j_dict_hasvalue(shm, obj, val);
		break;
	case JTYPE_LIST:
		result = j_list_hasvalue(shm, obj, val);
		break;
	default:
		PE("`jhasval` only works with dict/list objects");
		result = 0;
		// fall through to free val
	}

	if(free_val) j_free(shm, val);

	if(!result)
		goto _fail;
	// else, normal execution

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

char *jhasval_doc[] = {
	"jhasval <handler> <JSON|handler>",
	"",
	"test if a dict or list has a value",
	NULL
};

struct builtin jhasval_struct = {
	"jhasval",
	jhasval_builtin,
	BUILTIN_ENABLED,
	jhasval_doc,
	"jhasval <handler> <JSON|handler>",
	0
};
