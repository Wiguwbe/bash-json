
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
