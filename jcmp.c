
#include "common.h"

int jcmp_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;
	list = loptend;
	if(!list)
		return EX_USAGE;
	if(list->next)
		if(list->next->next)
			// 3+ arguments
			return EX_USAGE;


	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		fprintf(stderr, "error: failed to open shared memory\n");
		return EXECUTION_FAILURE;
	}

	long obj_a = -1, obj_b = -1;
	int free_a = 0, free_b = 0;

	// one of the arguments is always from CLI
	{
		char *obj_str = list->word->word;
		if(is_handler(obj_str))
		{
			obj_a = get_handler(obj_str);
		}
		if(obj_a < 0)
		{
			// try JSON literal
			obj_a = j_parse_buffer(shm, obj_str, strlen(obj_str), 1);
			free_a = 1;
		}
		if(obj_a < 0)
		{
			// fallback to string
			obj_a = j_str_new(shm, obj_str);
			free_a = 1;
		}
		if(obj_a < 0)
		{
			PE("unable to get JSON object");
			goto _fail;
		}
		list = list->next;
	}
	// second object
	if(list)
	{
		char *obj_str = list->word->word;
		if(is_handler(obj_str))
		{
			obj_b = get_handler(obj_str);
		}
		if(obj_b < 0)
		{
			obj_b = j_parse_buffer(shm, obj_str, strlen(obj_str), 1);
			free_b = 1;
		}
		if(obj_b < 0)
		{
			// fallback
			obj_b = j_str_new(shm, obj_str);
			free_b = 1;
		}
		if(obj_b < 0)
		{
			PE("unable to get JSON object");
			if(free_a)
				j_free(shm, obj_a);
			goto _fail;
		}
	}
	else if(!isatty(fileno(stdin)))
	{
		// read from stdin, tricky this one
		int slen = 0;
		char *obj_str = read_stdin_all(&slen);
		if(!obj_str)
		{
			PE("unable to allocate memory");
			if(free_a)
				j_free(shm, obj_a);
			goto _fail;
		}
		PD("from stdin (%d) '%s'", slen, obj_str);
		if(is_handler(obj_str))
		{
			obj_b = get_handler(obj_str);
		}
		if(obj_b < 0)
		{
			obj_b = j_parse_buffer(shm, obj_str, slen, 1);
			free_b = 1;
		}
		if(obj_b < 0)
		{
			// fallback
			obj_b = j_str_new(shm, obj_str);
			free_b = 1;
		}
		free(obj_str);
		if(obj_b < 0)
		{
			PE("unable to get JSON object from STDIN");
			if(free_a)
				j_free(shm, obj_a);
			goto _fail;
		}
	}
	else
	{
		if(free_a) j_free(shm, obj_a);
		goto _usage;
	}

	PD("types %d %d", j_type(shm, obj_b), j_type(shm, obj_a));

	// compare them
	int result = j_cmp(shm, obj_a, obj_b);

	if(free_a) j_free(shm, obj_a);
	if(free_b) j_free(shm, obj_b);

	shmem_fini(shm);

	if(result)
		return EXECUTION_FAILURE;

	return EXECUTION_SUCCESS;

_usage:
	shmem_fini(shm);
	return EX_USAGE;
_fail:
	shmem_fini(shm);
	return EXECUTION_FAILURE;
}

// no load/unload

char *jcmp_doc[] = {
	"jcmp <JSON|handler> <JSON|handler>",
	"",
	"compare 2 JSON objects for equality, returns 1 if they differ",
	NULL
};

struct builtin jcmp_struct = {
	"jcmp",
	jcmp_builtin,
	BUILTIN_ENABLED,
	jcmp_doc,
	"jcmp <JSON|handler> <JSON|handler>",
	0
};
