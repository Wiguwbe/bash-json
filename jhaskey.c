
#include "common.h"

int jhaskey_builtin(WORD_LIST *list)
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
	char *key = NULL;

	if(list->next)
	{
		// 2 arguments, first is handler
		char *obj_str = list->word->word;
		if(!is_handler(obj_str))
		{
			PE("invalid handler");
			goto _usage;
		}
		obj = get_handler(obj_str);
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
		PE("invalid handler");
		goto _usage;
	}

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
	}

	if(!j_dict_haskey(shm, obj, key))
		goto _fail;

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

char *jhaskey_doc[] = {
	"jhaskey <handler> <key>",
	"",
	"test if a dict has the key",
	NULL
};

struct builtin jhaskey_struct = {
	"jhaskey",
	jhaskey_builtin,
	BUILTIN_ENABLED,
	jhaskey_doc,
	"jhaskey <handler> <key>",
	0
};
