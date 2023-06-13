
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"

int jload_builtin(WORD_LIST *list)
{
	if(no_options(list))
		return EX_USAGE;
	list = loptend;
	if(list)
		return EX_USAGE;

	void *shm = shmem_init(shm_name);
	if(!shm)
	{
		PE("failed to open shared memory");
		return EXECUTION_FAILURE;
	}

	long object = j_parse_file(shm, stdin, 0);
	if(object<0)
	{
		shmem_fini(shm);
		PE("failed to load JSON");
		return EXECUTION_FAILURE;
	}

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
