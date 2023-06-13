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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ctype.h>

#include "shmalloc.h"

#include "json.h"
#include "json-parser.h"

#ifdef PD
#undef PD
#endif
#ifdef DEBUG
#define PD(fmt, ...) fprintf(stderr, fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#else
#define PD(fmt, ...)
#endif


/*
	These functions and structs are adapted to work
	with the `shmalloc` allocator

	so these actually just returns longs, offsets into
	the shared memory where the object is
*/

/*
	Very basic "dict" implementation
	(it's to be used in shell commands, so speed
	can be, a little bit, discarded)
*/
struct j_dict_item {
	long ptr_next_item;
	long str_key;
	long ptr_value;
};

/*
	Linked list
*/
struct j_list_item {
	long ptr_next_item;
	long ptr_value;
};

struct j_value {
	int jtype;
	union {
		struct {
			long ptr_dict_head;
			long ptr_dict_tail;
			int dict_len;
		};
		struct {
			long ptr_list_head;
			long ptr_list_tail;
			int list_len;
		};
		long val_integer;
		double val_float;
		long str_val;
	};
};

/*
	Functions to implement:

	- new (all)
	- free (all)
	- type (all)
	- val (basic types)
	- get (dict, list)
	- set (dict, list)
	- del (dict, list)
	- len (dict, list)
	//- keys (dict)
	//- values (dict, list implicit)
	- iter (dict, list)
	- cmp (generic, dict, last)
	- haskey (dict inline)
	- hasval (dict, list)
*/

/*
	NEW functions
*/

// use these to store a single NULL, TRUE or FALSE
long ptr_j_null = -1;
long ptr_j_true = -1;
long ptr_j_false = -1;

static long _j_base_new(void *shm, long *store, int type)
{
	PD("_j_base_new");
	if(*store >= 0)
		return *store;
	long ptr = shmalloc(shm, sizeof(struct j_value));
	if(ptr<0)
		return ptr;
	struct j_value *jv = shpointer(shm, ptr);
	jv->jtype = type;
	return *store = ptr;
}

long j_null_new(void *shm)
{
	return _j_base_new(shm, &ptr_j_null, JTYPE_NULL);
}

long j_bool_new(void *shm, int val)
{
	return _j_base_new(shm, val?&ptr_j_true:&ptr_j_false, val?JTYPE_TRUE:JTYPE_FALSE);
}

long j_int_new(void *shm, long val)
{
	PD("j_int_new");
	long ptr_j_int = shmalloc(shm, sizeof(struct j_value));
	if(ptr_j_int < 0)
		return ptr_j_int;
	struct j_value *jv = shpointer(shm, ptr_j_int);
	jv->jtype = JTYPE_INT;
	jv->val_integer = val;
	return ptr_j_int;
}

long j_float_new(void *shm, double val)
{
	PD("j_float_new");
	long j_off = shmalloc(shm, sizeof(struct j_value));
	if(j_off<0)
		return -1;
	struct j_value *jv = shpointer(shm, j_off);
	jv->jtype = JTYPE_FLOAT;
	jv->val_float = val;
	return j_off;
}

long _j_str_new(void *shm, char *str, int size)
{
	long j_off = shmalloc(shm, sizeof(struct j_value));
	if(j_off<0)
		return -1;
	long s_off = shmalloc(shm, size+1);
	if(s_off<0)
	{
		shfree(shm, j_off);
		return -1;
	}
	memcpy(shpointer(shm, s_off), str, size);
	((char*)shpointer(shm, s_off))[size] = 0;
	struct j_value *jv = shpointer(shm, j_off);
	jv->jtype = JTYPE_STR;
	jv->str_val = s_off;
	return j_off;
}

long j_str_new(void *shm, char *str)
{
	if(!str)
		return -1;
	return _j_str_new(shm, str, strlen(str));
}

long j_list_new(void *shm)
{
	long j_off = shmalloc(shm, sizeof(struct j_value));
	if(j_off<0)
		return -1;
	struct j_value *jv = shpointer(shm, j_off);
	jv->jtype = JTYPE_LIST;
	jv->ptr_list_head = jv->ptr_list_tail = -1;
	jv->list_len = 0;
	return j_off;
}

long j_dict_new(void *shm)
{
	long j_off = shmalloc(shm, sizeof(struct j_value));
	if(j_off<0)
		return -1;
	struct j_value *jv = shpointer(shm, j_off);
	jv->jtype = JTYPE_DICT;
	jv->ptr_dict_head = jv->ptr_dict_tail = -1;
	jv->dict_len = 0;
	return j_off;
}

/*
	FREE functions
*/

void j_free(void *shm, long obj)
{
	struct j_value *jv = shpointer(shm, obj);
	switch(jv->jtype)
	{
		case JTYPE_NULL:
		case JTYPE_TRUE:
		case JTYPE_FALSE:
			// nothing to do here
			break;
		case JTYPE_INT:
		case JTYPE_FLOAT:
			// quite basic
			shfree(shm, obj);
			break;
		case JTYPE_STR:
			j_str_free(shm, obj);
			break;
		case JTYPE_LIST:
			j_list_free(shm, obj);
			break;
		case JTYPE_DICT:
			j_dict_free(shm, obj);
			break;
		// fuck the dEfAuLt
	}
}

void j_null_free(void *shm, long obj)
{
	// nothing here
	return;
}
void j_bool_free(void *shm, long obj)
{
	// nothing here
	return;
}

void j_int_free(void *shm, long obj)
{
	shfree(shm, obj);
}

void j_float_free(void *shm, long obj)
{
	shfree(shm, obj);
}

void j_str_free(void *shm, long obj)
{
	struct j_value *jv = shpointer(shm, obj);
	shfree(shm, jv->str_val);
	shfree(shm, obj);
}

void j_list_free(void *shm, long obj)
{
	// free internal list
	struct j_value *jv = shpointer(shm, obj);
	while(jv->ptr_list_head >= 0)
	{
		long ptr_this = jv->ptr_list_head;
		struct j_list_item *li = shpointer(shm, ptr_this);
		jv->ptr_list_head = li->ptr_next_item;
		j_free(shm, li->ptr_value);
		shfree(shm, ptr_this);
	}
	shfree(shm, obj);
}

void j_dict_free(void *shm, long obj)
{
	// free internal list
	struct j_value *jv = shpointer(shm, obj);
	while(jv->ptr_dict_head >= 0)
	{
		long ptr_this = jv->ptr_list_head;
		struct j_dict_item *di = shpointer(shm, ptr_this);
		jv->ptr_dict_head = di->ptr_next_item;
		shfree(shm, di->str_key);
		j_free(shm, di->ptr_value);
		shfree(shm, ptr_this);
	}
	shfree(shm, obj);
}

/*
	TYPE
*/

int j_type(void *shm, long obj)
{
	struct j_value *jv = shpointer(shm, obj);
	return jv->jtype;
}

/*
	VAL functions
*/

long j_int_val(void *shm, long obj)
{
	return ((struct j_value*)shpointer(shm, obj))->val_integer;
}

double j_float_val(void *shm, long obj)
{
	return ((struct j_value*)shpointer(shm, obj))->val_integer;
}

char *j_str_val(void *shm, long obj)
{
	return shpointer(shm, ((struct j_value*)shpointer(shm, obj))->str_val);
}


/*
	GET functions
*/

long j_list_get(void *shm, long obj, int index)
{
	if(index < 0)
		return -1;
	struct j_value *jv = shpointer(shm, obj);
	long ptr_item;
	struct j_list_item *li;
	for(ptr_item = jv->ptr_list_head; index; index--)
	{
		if(ptr_item < 0)
			return -1;
		li = shpointer(shm, ptr_item);
		ptr_item = li->ptr_next_item;
	}
	li = shpointer(shm, ptr_item);
	return li->ptr_value;
}

long j_dict_get(void *shm, long obj, char *key)
{
	struct j_value *jv = shpointer(shm, obj);
	long ptr_item;
	struct j_dict_item *di;
	for(ptr_item = jv->ptr_dict_head; ptr_item >= 0; ptr_item = di->ptr_next_item)
	{
		di = shpointer(shm, ptr_item);
		char *s = shpointer(shm, di->str_key);
		if(!strcmp(s, key))
			return di->ptr_value;
	}
	return -1;
}

/*
	SET functions
*/

int j_list_set(void *shm, long obj, int index, long value)
{
	struct j_value *jv = shpointer(shm, obj);
	// where to put it?
	if(index < 0)
	{
		// append
		long ptr_new_item = shmalloc(shm, sizeof(struct j_list_item));
		if(ptr_new_item < 0)
			return -1;
		jv = shpointer(shm, obj);
		struct j_list_item *ji = shpointer(shm, ptr_new_item);
		ji->ptr_value = value;
		ji->ptr_next_item = -1;
		if(jv->ptr_list_tail == -1)
			jv->ptr_list_head = jv->ptr_list_tail = ptr_new_item;
		else
		{
			struct j_list_item *tail = shpointer(shm, jv->ptr_list_tail);
			tail->ptr_next_item = ptr_new_item;
			jv->ptr_list_tail = ptr_new_item;
		}
		jv->list_len++;
		return 0;
	}
	else
	{
		// update
		long ptr_iter = jv->ptr_list_head;
		struct j_list_item *iter;
		for(ptr_iter = jv->ptr_list_head; ptr_iter >= 0; ptr_iter = iter->ptr_next_item)
		{
			iter = shpointer(shm, ptr_iter);
			if(!index)
			{
				// set it here
				// free previous
				j_free(shm, iter->ptr_value);
				iter->ptr_value = value;
				return 0;
			}
			index--;
		}
	}
	// index not found
	return -1;
}


int _j_dict_set(void *shm, long obj, char *key, int key_len, long value)
{
	struct j_value *jv = shpointer(shm, obj);
	// look for item first
	{
		long ptr_iter;
		struct j_dict_item *di;
		for(ptr_iter = jv->ptr_dict_head; ptr_iter >= 0; ptr_iter = di->ptr_next_item)
		{
			di = shpointer(shm, ptr_iter);
			char *s = shpointer(shm, di->str_key);
			if(!memcmp(s, key, key_len))
			{
				// update this one
				j_free(shm, di->ptr_value);
				di->ptr_value = value;
				return 0;
			}
		}
	}
	// key not found, create it
	{
		long ptr_new_item = shmalloc(shm, sizeof(struct j_dict_item));
		if(ptr_new_item < 0)
			return -1;
		long str_key = shmalloc(shm, key_len+1);
		if(str_key < 0)
		{
			shfree(shm, ptr_new_item);
			return -1;
		}
		// after malloc, re-get pointers
		jv = shpointer(shm, obj);
		memcpy(shpointer(shm, str_key), key, key_len);
		((char*)shpointer(shm, str_key))[key_len] = 0;
		struct j_dict_item *di = shpointer(shm, ptr_new_item);
		di->ptr_next_item = -1;
		di->str_key = str_key;
		di->ptr_value = value;
		// append to list
		if(jv->ptr_dict_tail < 0)
		{
			jv->ptr_dict_head = jv->ptr_dict_tail = ptr_new_item;
		}
		else
		{
			struct j_dict_item *tail = shpointer(shm, jv->ptr_dict_tail);
			tail->ptr_next_item = jv->ptr_dict_tail = ptr_new_item;
		}
		jv->dict_len++;
	}

	return 0;
}

int j_dict_set(void *shm, long obj, char *key, long value)
{
	return _j_dict_set(shm, obj, key, strlen(key), value);
}

/*
	DEL functions
*/

int j_list_del(void *shm, long obj, int index)
{
	if(index < 0)
		return -1;
	struct j_value *jv = shpointer(shm, obj);
	if(jv->ptr_list_head < 0)
		return -1;
	long ptr_iter;
	if(!index)	// index == 0
	{
		// it's the head
		ptr_iter = jv->ptr_list_tail;
		struct j_list_item *li;
		if(jv->ptr_list_tail == jv->ptr_list_head)
		{
			jv->ptr_list_head = jv->ptr_list_tail = -1;
		}
		else
		{
			// reuse var
			li = shpointer(shm, jv->ptr_list_head);
			jv->ptr_list_head = li->ptr_next_item;
		}
		li = shpointer(shm, ptr_iter);
		j_free(shm, li->ptr_value);
		shfree(shm, ptr_iter);
		jv->list_len --;
		return 0;
	}
	index--;	// skip one because we delete the one after
	for(ptr_iter = jv->ptr_list_head; index; index--)
	{
		struct j_list_item *li = shpointer(shm, ptr_iter);
		ptr_iter = li->ptr_next_item;
		// also, check after
		if(ptr_iter < 0)
			return -1;
	}
	// delete it
	{
		struct j_list_item *prev = shpointer(shm, ptr_iter);
		long ptr_delete = prev->ptr_next_item;
		struct j_list_item *to_delete = shpointer(shm, ptr_delete);
		prev->ptr_next_item = to_delete->ptr_next_item;
		if(prev->ptr_next_item < 0)
		{
			// we deleted the tail
			jv->ptr_list_tail = ptr_iter;
		}
		j_free(shm, to_delete->ptr_value);
		shfree(shm, ptr_delete);
	}
	jv->list_len --;
	return 0;
}

int j_dict_del(void *shm, long obj, char*key)
{
	struct j_value *jv = shpointer(shm, obj);
	if(jv->ptr_dict_head < 0)
		return -1;
	long ptr_iter, ptr_prev = -1;
	struct j_dict_item *di;
	for(ptr_iter = jv->ptr_dict_head; ptr_iter >= 0; ptr_prev = ptr_iter, ptr_iter = di->ptr_next_item)
	{
		di = shpointer(shm, ptr_iter);
		if(strcmp(shpointer(shm, di->str_key), key))
		{
			// not it, continue
			continue;
		}
		// to leave more space to actually delete it
		if(ptr_prev < 0)
		{
			// deleting HEAD
			jv->ptr_dict_head = di->ptr_next_item;
		}
		else
		{
			struct j_dict_item *prev = shpointer(shm, ptr_prev);
			prev->ptr_next_item = di->ptr_next_item;
			if(ptr_iter == jv->ptr_dict_tail)
				jv->ptr_dict_tail = ptr_prev;
		}
		j_free(shm, di->ptr_value);
		shfree(shm, di->str_key);
		shfree(shm, ptr_iter);
		jv->dict_len --;
		return 0;
	}
	// else, not found
	return -1;
}

/*
	LEN functions
*/

int j_list_len(void *shm, long obj)
{
	return ((struct j_value*)shpointer(shm, obj))->list_len;
}

int j_dict_len(void *shm, long obj)
{
	return ((struct j_value*)shpointer(shm, obj))->dict_len;
}

/*
	ITER functions
*/

int j_list_iter(void *shm, long obj, int (*callback)(void *, int, long, void *), void *user_data)
{
	struct j_value *jv = shpointer(shm, obj);
	long ptr_iter;
	struct j_list_item *li;
	int index = 0;
	int ret;
	for(ptr_iter = jv->ptr_list_head; ptr_iter>=0; ptr_iter = li->ptr_next_item)
	{
		li = shpointer(shm, ptr_iter);
		if((ret=callback(shm, index, li->ptr_value, user_data))!= 0)
			return ret;
		// in case there were `shmalloc`s there
		li = shpointer(shm, ptr_iter);
	}
	return 0;
}

int j_dict_iter(void *shm, long obj, int (*callback)(void *, char *, long, void *), void *user_data)
{
	struct j_value *jv = shpointer(shm, obj);
	long ptr_iter;
	struct j_dict_item *di;
	for(ptr_iter = jv->ptr_dict_head; ptr_iter >= 0; ptr_iter = di->ptr_next_item)
	{
		di = shpointer(shm, ptr_iter);
		char *s = shpointer(shm, di->str_key);
		int klen = strlen(s)+1;
		char *key = malloc(klen);
		if(!key)
			return -1;
		memcpy(key, s, klen);
		// reuse var name
		klen = callback(shm, key, di->ptr_value, user_data);
		free(key);
		if(klen)
			return klen;
		di = shpointer(shm, ptr_iter);
	}

	return 0;
}

/*
	CMP functions
*/
int j_cmp(void *shm, long a, long b)
{
	struct j_value *ja = shpointer(shm, a);
	struct j_value *jb = shpointer(shm, b);
	int ta = ja->jtype;
	int tb = jb->jtype;
	if(ta!=tb)
		// this also solves NULL, TRUE, FALSE
		return 1;
	switch(ta)
	{
		case JTYPE_INT:
			PD("cmp %d %d", ja->val_integer, jb->val_integer);
			return ja->val_integer - jb->val_integer;
		case JTYPE_FLOAT:
			// because equal is 0
			PD("cmp %f %f", ja->val_float, jb->val_float);
			return !(ja->val_float == jb->val_float);
		case JTYPE_STR:
			PD("cmp '%s' '%s'", shpointer(shm, ja->str_val), shpointer(shm, jb->str_val));
			return strcmp(shpointer(shm, ja->str_val), shpointer(shm, jb->str_val));
		case JTYPE_LIST:
			return j_list_cmp(shm, a,b);
		case JTYPE_DICT:
			return j_dict_cmp(shm, a,b);
	}
	return -1;
}

int j_list_cmp(void *shm, long a, long b)
{
	struct j_value *ja = shpointer(shm, a);
	struct j_value *jb = shpointer(shm, b);
	if(ja->list_len != jb->list_len)
		return 1;
	long a_iter, b_iter;
	struct j_list_item *la, *lb;
	for(a_iter = ja->ptr_list_head, b_iter = jb->ptr_list_head; a_iter >=0; a_iter = la->ptr_next_item, b_iter = lb->ptr_next_item)
	{
		la = shpointer(shm, a_iter);
		lb = shpointer(shm, b_iter);
		if(j_cmp(shm, la->ptr_value, lb->ptr_value))
			return 1;
	}
	return 0;
}

static int _dict_cmp(void *shm, char *key, long val_a, void *user_data)
{
	long b =*((long*)user_data);
	// iterate over A, check value in B is equal
	long val_b = j_dict_get(shm, b, key);
	if(val_b < 0)
		return 1;
	return j_cmp(shm, val_a, val_b);
}

int j_dict_cmp(void *shm, long a, long b)
{
	struct j_value *ja = shpointer(shm, a);
	struct j_value *jb = shpointer(shm, b);
	if(ja->dict_len != jb->dict_len)
		return 1;
	return j_dict_iter(shm, a, _dict_cmp, &b);
}

/*
	HAS functions
*/

static int _list_has(void *shm, int index, long val, void *search)
{
	// return 1 if TRUE
	return !j_cmp(shm, val, *((long*)search));
}

int j_list_hasvalue(void *shm, long obj, long search)
{
	return j_list_iter(shm, obj, _list_has, &search);
}

static int _dict_has(void *shm, char *key, long val, void *search)
{
	return !j_cmp(shm, val, *((long*)search));
}

int j_dict_hasvalue(void *shm, long obj, long search)
{
	return j_dict_iter(shm, obj, _dict_has, &search);
}

/*
	Parser stuff
*/

// linkedlist for parsing callback
struct parser_ll {
	int type;	// JTYPE_DICT or JTYPE_LIST
	struct parser_ll *prev, *next;
	union {
		struct {
			long ptr_dict;
			char* key;
			size_t key_len;
		};
		struct {
			long ptr_list;
		};
	};
};

struct parser_data {
	struct parser_ll *tail;
	long top_level;
	void *shm;
};

static int _parser_callback(JSON_TYPE type, const char *value, size_t size, void *user_data)
{
	struct parser_data *pd = (struct parser_data*)user_data;
	long obj = -1;
	PD("got key %d", type);
	switch(type)
	{
	case JSON_NULL:
		obj = j_null_new(pd->shm);
		break;
	case JSON_FALSE:
	case JSON_TRUE:
		obj = j_bool_new(pd->shm, type==JSON_TRUE);
		break;
	case JSON_NUMBER: {
		int a,b,c,d;
		json_analyze_number(value, size, &a, &b, &c, &d);
		if(c)	// we only really care about this one
		{
			// integer
			obj = j_int_new(pd->shm, json_number_to_int64(value, size));
		}
		else
		{
			double double_val;
			if(json_number_to_double(value, size, &double_val))
			{
				fprintf(stderr, "error: number is to big to fit on float\n");
				return 1;
			}
			obj = j_float_new(pd->shm, double_val);
		}
		break;
	}
	case JSON_STRING:
		obj = _j_str_new(pd->shm, value, size);
		break;
	case JSON_KEY: {
		struct parser_ll *node = pd->tail;
		node->key = value;
		node->key_len = size;
		return 0;
	}
	case JSON_ARRAY_BEG: {
		obj = j_list_new(pd->shm);
		if(obj<0)
			return -2;
		struct parser_ll *node = (struct parser_ll*)malloc(sizeof(struct parser_ll));
		if(!node)
		{
			j_free(pd->shm, obj);
			return -2;
		}
		if(pd->tail)
			pd->tail->next = node;
		node->prev = pd->tail;
		node->next = NULL;
		pd->tail = node;
		node->type = JTYPE_LIST;
		node->ptr_list = obj;
		break;
	}
	case JSON_ARRAY_END: {
		// pop from list
		struct parser_ll *save = pd->tail;
		obj = pd->tail->ptr_list;
		pd->tail = pd->tail->prev;
		free(save);
		return 0;
	}
	case JSON_OBJECT_BEG: {
		PD("a");
		obj = j_dict_new(pd->shm);
		if(obj<0)
			return -2;
		PD("obj: %ld", obj);
		struct parser_ll *node = (struct parser_ll*)malloc(sizeof(struct parser_ll));
		if(!node)
		{
			j_free(pd->shm, obj);
			return -2;
		}
		PD("node: %p", node);
		if(pd->tail)
			pd->tail->next = node;
		node->prev = pd->tail;
		node->next = NULL;
		pd->tail = node;
		node->type = JTYPE_DICT;
		node->ptr_dict = obj;
		break;
	}
	case JSON_OBJECT_END: {
		struct parser_ll *save = pd->tail;
		obj = pd->tail->ptr_dict;
		pd->tail = pd->tail->prev;
		free(save);
		return 0;
	}
	}

	// add to parent or top-level
	PD("obj is %ld", obj);
	if(obj<0)
		return -2;
	if(pd->top_level<0)
		pd->top_level = obj;
	// add to parent
	else
	{
		struct parser_ll *node = pd->tail;
		if(type == JSON_OBJECT_BEG||type==JSON_ARRAY_BEG)
			node = node->prev;
		if(node)
		{
			PD("test 1");
			if(node->type == JTYPE_LIST)
			{
				PD("test 1.2");
				j_list_set(pd->shm, node->ptr_list, -1, obj);
			}
			else
			{
				PD("test 1.3");
				_j_dict_set(pd->shm, node->ptr_dict, node->key, node->key_len, obj);
			}
			PD("test 2");
		}
	}

	return 0;
}

long j_parse_file(void *shm, FILE *file, int suppress_error)
{
	JSON_PARSER parser;
	JSON_CALLBACKS callbacks;
	JSON_INPUT_POS pos;
	char buffer[1024];
	struct parser_data user_data = {NULL, -1, shm};
	callbacks.process = _parser_callback;
	json_init(&parser, &callbacks, NULL, &user_data);
	int l;
	int err;
	while(!feof(file))
	{
		l = fread(buffer, 1, 1024, file);
		if(l<1024 && ferror(file))
		{
			if(user_data.top_level >= 0)
				j_free(shm, user_data.top_level);
			if(!suppress_error)
				fprintf(stderr, "error reading from input\n");
			return -1;
		}
		if((err = json_feed(&parser, buffer, l))!=0)
		{
			json_fini(&parser, &pos);
			if(!suppress_error)
				fprintf(stderr, "error parsing json, at line %d, column %d: %s\n", pos.line_number, pos.column_number, json_error_str(err));
			if(user_data.top_level >= 0)
				j_free(shm, user_data.top_level);
			return -1;
		}
	}
	if((err = json_fini(&parser, &pos))!=0)
	{
		if(!suppress_error)
			fprintf(stderr, "error parsing json, at line %d, column %d: %s\n", pos.line_number, pos.column_number, json_error_str(err));
		if(user_data.top_level>=0)
			j_free(shm, user_data.top_level);
		return -1;
	}
	return user_data.top_level;
}

long j_parse_buffer(void *shm, char *buffer, int len, int suppress_error)
{
	FILE *file = fmemopen(buffer, len, "rb");
	if(!file)
		return -1;
	long out = j_parse_file(shm, file, suppress_error);
	fclose(file);
	return out;
}
