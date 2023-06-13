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

#ifndef _SHMALLOC_H_
#define _SHMALLOC_H_


/*
	Initialize handler for shmem

	It takes the name for the shared memory

	Instead of using opaque structs, we simply use `void *`
*/
void *shmem_init(char *name);

/*
	Free handler

	Takes the handler pointer

	This does NOT delete the shared memory area, to delete it
	(currently) use the `shmem_destroy` function
*/
void shmem_fini(void *handler);

/*
	Destroy the shared memory

	returns -1 on failure and sets errno
	(see shm_unlink(3) for more info)
*/
int shmem_destroy(char *name);


/*
	Allocate a block of memory and return an offset into the
	shared memory.

	It takes the handler and the desired size of the block

	To make use of this offset, use the `shpointer` function

	Returns -1 on error
*/
long shmalloc(void *handler, unsigned long size);

/*
	Free a memory block, doesn't report errors
*/
void shfree(void *handler, long offset);

/*
	Get an absolute pointer to the memory block

	Because the base address may change on a call to `shmalloc`,
	these pointer SHOULD be short lived
*/
void *shpointer(void *handler, long offset);


#endif
