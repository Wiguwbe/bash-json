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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>

#include "shmalloc.h"

//static void _print_shmem(struct shmem *);

struct shmem_block {
	long next_offset;
	unsigned long size;
};

struct shmem {
	void *base_ptr;
	int fd;
	unsigned long size;
};

#ifdef DEBUG
#define PD(fmt, ...) fprintf(stderr, fmt "\n", __VA_ARGS__)
#else
#define PD(fmt, ...)
#endif

/*
	Init shared memory allocator
*/
void *shmem_init(char *name)
{
	struct shmem *ret = (struct shmem*)malloc(sizeof(struct shmem));
	struct stat stat;
	int init_size = 0;
	if(!ret)
		return NULL;
	if((ret->fd = shm_open(name, O_CREAT|O_RDWR, 0600)) < 0)
	{
		free(ret);
		return NULL;
	}
	if(fstat(ret->fd, &stat))
	{
		close(ret->fd);
		free(ret);
		return NULL;
	}
	init_size = ret->size = stat.st_size;

	// allocate HEAD
	if(!ret->size)
	{
		if(ftruncate(ret->fd, sizeof(struct shmem_block)))
		{
			close(ret->fd);
			free(ret);
			return NULL;
		}
		ret->size = sizeof(struct shmem_block);
	}
	if((ret->base_ptr = mmap(NULL, ret->size, PROT_READ|PROT_WRITE, MAP_SHARED, ret->fd, 0))==MAP_FAILED)
	{
		close(ret->fd);
		free(ret);
		return NULL;
	}
	// setup HEAD
	if(!init_size)
	{
		struct shmem_block *head = (struct shmem_block*)ret->base_ptr;
		head->size = 0;
		head->next_offset = -1;
	}
	return (void*)ret;
}

/*
	closes the file descriptor and unmaps the memory
*/
void shmem_fini(void *handler)
{
	struct shmem *h = (struct shmem*)handler;
	munmap(h->base_ptr, h->size);
	close(h->fd);
}

/*
	Utility function to destroy a shared memory object
	to avoid the need for the user to include the shm (mman)
	headers
*/
int shmem_destroy(char *name)
{
	return shm_unlink(name);
}

/*
	utility function to expand the shared memory
	and the MMAP.

	pointers after this function may be invalidated
*/
static int _expand_shm(struct shmem *handler, unsigned long size)
{
	void *ptr;
	if(ftruncate(handler->fd, handler->size+size))
		return 1;
	// remap
	ptr = mremap(handler->base_ptr, handler->size, handler->size+size, MREMAP_MAYMOVE);
	if(ptr==MAP_FAILED)
	{
		// reset shm size
		ftruncate(handler->fd, handler->size);
		return 1;
	}
	handler->size += size;
	handler->base_ptr = ptr;
	return 0;
}

/*
	Allocates a memory block and returns an offset into the shared memory area

	To get an actual pointer, use the `shpointer` function
*/
long shmalloc(void *handler, unsigned long size)
{
	if(!size)
		return -1;
	struct shmem *h = (struct shmem*)handler;
	unsigned long actual_size = size + sizeof(struct shmem_block);
	unsigned long space_left = h->size;
	unsigned long this_offset = 0;

	// align 8
	if(actual_size&7)
		actual_size += 8-(actual_size&7);

	// 1. check if there is space inside
	/* with the HEAD, the size is always >0 */
	{
		long next_offset = -1;
		struct shmem_block *block;
		while(this_offset >= 0)
		{
			block = shpointer(handler, this_offset);
			// don't count holes
			space_left = h->size - (this_offset + block->size + sizeof(struct shmem_block));
			next_offset = block->next_offset;
			if(next_offset>=0)
			{
				// check space between this one and the next one
				unsigned long end_of_this = this_offset + block->size + sizeof(struct shmem_block);
				unsigned long space_between = next_offset - end_of_this;
				if(space_between >= actual_size)
				{
					// we found it, cool
					PD("found hole for new block at %ld (this_offset %d, actual_size %d, space_between %d)", end_of_this, this_offset, actual_size, space_between);
					struct shmem_block *new_block = (struct shmem_block*)shpointer(handler, end_of_this);
					new_block->size = actual_size - sizeof(struct shmem_block);
					new_block->next_offset = next_offset;
					block->next_offset = end_of_this;
					return end_of_this + sizeof(struct shmem_block);
				}
				// else, keep looking
			}
			// prepare for next iteration
			if(next_offset<0)
			{
				// leave `this_offset` untouched
				break;
			}
			this_offset = next_offset;
		}
		// nothing between blocks, check after the blocks
		// look for space after the last one
		if(space_left >= actual_size)
		{
			unsigned long end_of_this = this_offset + block->size + sizeof(struct shmem_block);
			PD("allocating block at end of memory %ld", end_of_this);
			struct shmem_block *new_block = (struct shmem_block*)shpointer(handler, end_of_this);
			new_block->size = actual_size - sizeof(struct shmem_block);
			new_block->next_offset = -1;
			struct shmem_block *prev_block = (struct shmem_block*)shpointer(handler, this_offset);
			prev_block->next_offset = end_of_this;
			return end_of_this + sizeof(struct shmem_block);
		}
	}
	// 2. either no space, or is empty, allocate more memory
	{
		// offset after last memory region, may still be inside the allocate region
		unsigned long next_offset = h->size - space_left;
		unsigned long to_expand = actual_size - space_left;
		PD("expanding SHM by %d bytes (this_block%d, prev_block %d)", to_expand, next_offset, this_offset);
		if(_expand_shm(h, to_expand))
			return -1;
		// else, carry on
		struct shmem_block *new_block = shpointer(h, next_offset);
		new_block->size = actual_size - sizeof(struct shmem_block);
		new_block->next_offset = -1;
		struct shmem_block *prev_block = shpointer(h, this_offset);
		prev_block->next_offset = next_offset;
		return next_offset + sizeof(struct shmem_block);
	}
}

/*
	Free allocated block
*/
void shfree(void *handler, long offset)
{
	struct shmem *h = (struct shmem*)handler;
	// because we have HEAD and the next structure, offset must always be:
	if(offset<(sizeof(struct shmem_block)<<1) || offset >= h->size)
		return;
	unsigned int this_offset = 0;
	while(1)
	{
		struct shmem_block *block = shpointer(handler, this_offset);
		if(block->next_offset < 0)
			break;
		if(offset == (block->next_offset + sizeof(struct shmem_block)))
		{
			// found it
			struct shmem_block *target = shpointer(handler, block->next_offset);
			block->next_offset = target->next_offset;
			// no need to "free", but for debug purposes, we'll zero it
			memset(target, 0, target->size + sizeof(struct shmem_block));
			break;
		}
		/* else */
		if(offset < (block->next_offset + sizeof(struct shmem_block)))
		{
			// overshoot, it's invalid
			break;
		}
		this_offset = block->next_offset;
	}
	// all cool
	return;
}

/*
	Get a pointer to the offset

	These pointers are volatile, `shmalloc` may move
	the base pointer and invalidate these ones
*/
void *shpointer(void *handler, long offset)
{
	return (void*)((struct shmem*)handler)->base_ptr+offset;
}

/*
	Utility to print the shared memory block,
	mostly for debugging purposes
*/
static void _print_shmem(struct shmem *handler)
{
	// print memory in hex, 8 by 8 (word by word)
	unsigned offset = 0;
	// iterate as array of words (long)
	unsigned long *ptr = (unsigned long*)handler->base_ptr;
	unsigned count = handler->size >> 3;
	fprintf(stderr, "++++++++++++++++\n");
	for(unsigned i=0;i<count;i++, offset+=8)
	{
		fprintf(stderr, "%016lx: %016lx\n", offset, ptr[i]);
	}
	fprintf(stderr, "----------------\n");
}

/*
	TEST AREA
*/
#ifdef TEST

/*
	we can use only (unsigned) longs arrays
	and to test it
*/

/*
	This tests mostly the shmalloc and shfree

	Tests to execute:
	A) no space left, expand and allocate in the expanded area
	B) allocate in a hole
	C) enough space at the end, allocate there
	D) some space left, expand and allocate

/*
	Steps to execute:
	1. init -> test empty/head
	2. allocate 16 bytes, should expand the shared memory size (populate with AAA...) (does A))
	3. allocate 16 bytes, should expand again (populate with BBB...) (does A) again)
	4. free block from 2., should create empty hole (zeroed)
	5. allocate 8 bytes, should use empty hole from 4. (populate with CCC...) (does B))
	6. free block from 3., should create empty space at the end (24 bytes)
	7. allocate 16 bytes, should use part of the available size (at the end) (populate with DDD...) (does C))
	8. allocate 16 bytes, should use the rest of memory available and expand 8 bytes (populate with EEE...) (does D))
	9. clear/finish
*/

#define TDS(td) sizeof(td)

// 1. empty (just HEAD)
static long td_1[] = {
	// HEAD
	-1,
	0,
	// DATA 0
};
static int td_1_s = TDS(td_1);

// 2. allocate 16 bytes
static long td_2[] = {
	// HEAD
	16,	// next offset
	0,	// size
	// NO DATA
	// BLOCK 1
	-1,	// next offset
	16,	// size
	// DATA 1
	0x4141414141414141,
	0x4141414141414141,
};
static int td_2_s = TDS(td_2);

// 3. allocate 16 bytes (again)
static long td_3[] = {
	// HEAD
	16,
	0,
	// BLOCK 1
	48,	// next offset
	16,	// size
	// DATA 1
	0x4141414141414141,
	0x4141414141414141,
	// BLOCK 2
	-1,	// next offset
	16,	// size
	// DATA 2
	0x4242424242424242,
	0x4242424242424242,
};
static int td_3_s = TDS(td_3);

// 4. free from 2.
static long td_4[] = {
	// HEAD
	48,
	0,
	// EMPTY SPACE
	0,
	0,
	0,
	0,
	// BLOCK 2
	-1,
	16,
	0x4242424242424242,
	0x4242424242424242,
};
static int td_4_s = TDS(td_4);

// 5. allocate 8 bytes, use hole
static long td_5[] = {
	// HEAD
	16,
	0,
	// BLOCK 3
	48,
	8,
	0x4343434343434343,
	// EMPTY SPACE
	0,
	// BLOCK 2
	-1,
	16,
	0x4242424242424242,
	0x4242424242424242,
};
static int td_5_s = TDS(td_5);

// 6. free block from 3.
static long td_6[] = {
	// HEAD
	16,
	0,
	// BLOCK 3
	-1,
	8,
	0x4343434343434343,
	// EMPTY SPACE
	0,
	0,
	0,
	0,
	0,
};
static int td_6_s = TDS(td_6);

// 7, allocate 16 bytes
static long td_7[] = {
	// HEAD
	16,
	0,
	// BLOCK 3
	40,
	8,
	0x4343434343434343,
	// BLOCK 4
	-1,
	16,
	0x4444444444444444,
	0x4444444444444444,
	// empty space
	0,
};
static int td_7_s = TDS(td_7);

// 8. sllocate 16 bytes, should expand
static long td_8[] = {
	// HEAD
	16,
	0,
	// BLOCK 3
	40,
	8,
	0x4343434343434343,
	// BLOCK 4
	72,
	16,
	0x4444444444444444,
	0x4444444444444444,
	// BLOCK 5
	-1,
	16,
	0x4545454545454545,
	0x4545454545454545,
};
static int td_8_s = TDS(td_8);

// 9. no data

static char *_td_fill =
	"AAAAAAAA"
	"AAAAAAAA"
	"BBBBBBBB"
	"BBBBBBBB"
	"CCCCCCCC"
	"DDDDDDDD"
	"DDDDDDDD"
	"EEEEEEEE"
	"EEEEEEEE"
;

static void _compare(struct shmem *mem, long *td, int td_s)
{
	fprintf(stderr, "SHMEM\n");
	_print_shmem(mem);
	int td_ss = td_s / sizeof(long);
	fprintf(stderr, "TD\n++++++++++++++++\n");
	for(unsigned long offset=0, i=0; i<td_ss; i++, offset+=8)
		fprintf(stderr, "%016lx: %016lx\n", offset, td[i]);
	fprintf(stderr, "----------------\n");
}

#define TEST_ALLOC(number, asize, fill) \
	long tc_ ## number = shmalloc(shmem, (asize));\
	if(tc_ ## number <0) { perror(#number ": failed to allocate memory\n"); goto _error; } \
	memcpy(shpointer(shmem, tc_ ## number) , fill, asize); \
	if(shmem->size!=td_ ## number ## _s || memcmp(shmem->base_ptr, td_ ## number , td_ ## number ## _s)) { \
		fprintf(stderr, "error: TEST CASE " #number " fails! (%ld)\n", tc_ ## number ); \
		_compare(shmem, td_ ## number , td_ ## number ## _s); \
		goto _error; \
	}

#define TEST_FREE(number, prev) \
	shfree(shmem, tc_ ## prev ); \
	if(shmem->size!=td_ ## number ## _s||memcmp(shmem->base_ptr, td_ ## number , td_ ## number ## _s)) { \
		fprintf(stderr, "error: TEST CASE " #number " fails!\n"); \
		_compare(shmem, td_ ## number , td_ ## number ## _s); \
		goto _error; \
	}

int main(int argc, char**argv)
{
	// 1.
	struct shmem *shmem = (struct shmem*)shmem_init("/dred");
	if(!shmem)
	{
		perror("1: failed to initialize:");
		return 1;
	}
	if(shmem->size!=td_1_s||memcmp(shmem->base_ptr, td_1, td_1_s))
	{
		fprintf(stderr, "error: TEST CASE 1 fails!\n");
		_compare(shmem, td_1, td_1_s);
		goto _error;
	}
	// 2. allocate 16 bytes
//
//	long tc_2 = shmalloc(shmem, 16);
//	if(tc_2<0)
//	{
//		perror("2: failed to allocate memory\n");
//		goto _error;
//	}
//	if(shmem->size!=td_2_s||memcmp(shmem->base_ptr, td_2, td_2_s))
//	{
//		fprintf(stderr, "error: TEST CASE 2 fails (%ld)!\n", tc_2);
//		_compare(shmem, td_2, td_2_s);
//		goto _error;
//	}
	TEST_ALLOC(2, 16, _td_fill)
//	// 3. allocate 16 bytes, again
//	long tc_3 = shmalloc(shmem, 16);
//	if(tc_3 < 0)
//	{
//		perror("3: failed to allocate memory\n");
//		goto _error;
//	}
//	if(shmem->size!=td_3_s||memcmp(shmem->base_ptr, td_3, td_3_s))
//	{
//		fprintf(stderr, "error: TEST CASE 3 fails!\n");
//		_compare(shmem, td_3, td_3_s);
//		goto _error;
//	}
	TEST_ALLOC(3, 16, _td_fill+16)
//	// 4. free from 2
//	shfree(shmem, tc_2);
//	if(shmem->size!=td_4_s||memcmp(shmem->base_ptr, td_4, td_4_s))
//	{
//		fprintf(stderr, "error: TEST CASE 4 fails!\n");
//		_compare(shmem, td_4, td_4_s);
//		goto _error;
//	}
//
	TEST_FREE(4, 2)
//	// 5. allocate in hole
//	long tc_5 = shmalloc(shmem, 8);
//	if(tc_5<0)
//	{
//		perror("5: failed to allocate memory\n");
//		goto _error;
//	}
//	if(shmem->size!=td_5_s||memcmp(shmem->base_ptr, td_5, td_5_s))
//	{
//		fprintf(stderr, "error: TEST CASE 5 fails!\n");
//		_compare(shmem, td_5, td_5_s);
//		goto _error;
//	}
	TEST_ALLOC(5, 8, _td_fill+32)
//	// 6. free from 3.
//	shfree(shmem, tc_3);
//	if(shmem->size!=td_6_s||memcmp(shmem->base_ptr, td_6, td_6_s))
//	{
//		fprintf(stderr, "error: TEST CASE 6 fails!\n");
//		_compare(shmem, td_6, td_6_s);
//		goto _error;
//	}
	TEST_FREE(6, 3)
//	// 7. allocate 16
//	long tc_7 = shmalloc(shmem, 16);
//	if(tc_7<0)
//	{
//		perror("7: failed to allocate memory\n");
//		goto _error;
//	}
//	if(shmem->size!=td_7_s||memcmp(shmem->base_ptr, td_7, td_7_s))
//	{
//		fprintf(stderr, "error: TEST CASE 7 fails!\n");
//		_compare(shmem, td_7, td_7_s);
//		goto _error;
//	}
	TEST_ALLOC(7, 16, _td_fill+40)
//	// 8. allocate 16
//	long tc_8 = shmalloc(shmem, 16);
//	if(tc_8<0)
//	{
//		perror("8: failed to allocate memory\n");
//		goto _error;
//	}
//	if(shmem->size!=td_8_s||memcmp(shmem->base_ptr, td_8, td_8_s))
//	{
//		fprintf(stderr, "error: TEST CASE 8 fails!\n");
//		_compare(shmem, td_8, td_8_s);
//		goto _error;
//	}
	TEST_ALLOC(8, 16, _td_fill+56)
	// 9 clean
	shmem_fini(shmem);
	shmem_destroy("/dred");

	fprintf(stderr, "All test cases passed!\n");

	return 0;

_error:
	shmem_fini(shmem);
	shmem_destroy("/dred");
	return 1;
}

#endif
