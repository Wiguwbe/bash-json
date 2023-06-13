
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"

#include "json-parser.h"

char shm_name[] = {0};

int count = 0;

__attribute__((constructor))
void _j_builtins_init(void)
{
	sprintf(shm_name, "/%lu", getpid());
}

__attribute__((destructor))
void _j_builtins_fini(void)
{
	if(getpid() == atol(shm_name+1))
		shmem_destroy(shm_name);
}

int init_top_level(void)
{
	return 0;
}


void fini_top_level(void)
{
	return ;
}


int is_handler(char *s)
{
	return s[0] == 'j' && s[1] == ':';
}

long get_handler(char *s)
{
	long r = atol(s+2);
	if(!r)
		return -1;
	return r;
}

long get_handler_stdin(void)
{
	long r;
	if(scanf("j:%ld", &r)<1)
		return -1;
	PD("handler is %ld", r);
	return r;
}

static int _do_print(const char *data, size_t size, void *unused)
{
	return printf("%.*s\n", size, data) != (size+1);
}

static int _do_print_str(const char *data, size_t size, void *unused)
{
	int *v = (int*)unused;
	if(data[0] == '"')
	{
		if(*v != 0)
			putchar(10);
		(*v) ++;
		return 0;
	}
	// skip quotes when printing strings
	return printf("%.*s", size, data) != (size);
}

void print_handler(void *shm, long obj)
{
	switch(j_type(shm, obj))
	{
	case JTYPE_DICT:
	case JTYPE_LIST:
		printf("j:%ld\n", obj);
		break;
	case JTYPE_NULL:
		puts("null");
		break;
	case JTYPE_FALSE:
		puts("false");
		break;
	case JTYPE_TRUE:
		puts("true");
		break;
	case JTYPE_INT:
		printf("%ld\n", j_int_val(shm, obj));
		break;
	case JTYPE_FLOAT:
		json_dump_double(j_float_val(shm, obj), _do_print, NULL);
		break;
	case JTYPE_STR: {
		char *s = j_str_val(shm, obj);
		int c = 0;
		int r = json_dump_string(s, strlen(s), _do_print_str, &c);
		break;
	}
	}
}

char *read_stdin_all(int *len)
{
	char *ptr = (char*)malloc(128);
	if(!ptr)
		return NULL;
	int read = 0;
	int size = 128;
	while(!feof(stdin))
	{
		// always leave space for an extra NULL byte
		int part = fread(ptr+read, 1, 127, stdin);
		read += part;
		if(part<127)
		{
			if(ferror(stdin))
			{
				free(ptr);
				return NULL;
			}
			break;
		}
		char *nptr = (char*)realloc(ptr, size+part);
		if(!nptr)
		{
			free(ptr);
			return NULL;
		}
		memset(ptr+size, 0, part);
		size += part;
		ptr = nptr;
	}
	*len = read;
	return ptr;
}
