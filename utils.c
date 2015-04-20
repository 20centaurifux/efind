#include <stdio.h>

#include "utils.h"

void *
utils_malloc(size_t size)
{
	void *ptr;

	if(!(ptr = malloc(size)))
	{
		perror("malloc()");
		abort();
	}

	return ptr;
}

void *
utils_realloc(void *ptr, size_t size)
{
	if(!(ptr = realloc(ptr, size)))
	{
		perror("realloc()");
		abort();
	}

	return ptr;
}

void
utils_free(void *ptr)
{
	if(ptr)
	{
		free(ptr);
	}
}

size_t
utils_next_pow2(size_t n)
{
	n -= 1;

	n = (n >> 1) | n;
	n = (n >> 2) | n;
	n = (n >> 4) | n;
	n = (n >> 8) | n;
	n = (n >> 16) | n;
	#if UINTPTR_MAX == 0xffffffffffffffff
	n = (n >> 32) | n;
	#endif

	return n + 1;
}


