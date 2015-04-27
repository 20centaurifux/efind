/***************************************************************************
    begin........: April 2015
    copyright....: Sebastian Fedrau
    email........: sebastian.fedrau@gmail.com
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License v3 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License v3 for more details.
 ***************************************************************************/
/**
   @file utils.c
   @brief Utility functions.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
   @version 0.1.0
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

char *
utils_whereis(const char *name)
{
	char *rest;
	char *token;
	char path[512];
	size_t len;
	int written;
	struct stat sb;

	rest = getenv("PATH");

	/* test if PATH is empty */
	if(!rest || !rest[0])
	{
		return NULL;
	}

	while((token = strtok_r(rest, ":", &rest)))
	{
		/* build absolute path to executable file */
		len = strlen(token);

		if(!len)
		{
			continue;
		}

		if(token[len - 1] == '/')
		{
			written = snprintf(path, 512, "%sfind", token);
		}
		else
		{
			written = snprintf(path, 512, "%s/find", token);
		}

		if(written >= 512)
		{
			fprintf(stderr, "%s: string truncation\n", __func__);
		}
		else
		{
			/* test if file does exist & can be executed by user */
			if(!stat(path, &sb))
			{
				if((sb.st_mode & S_IFMT) == S_IFREG && (sb.st_mode & S_IXUSR || sb.st_mode & S_IXGRP))
				{
					return strdup(path);
				}
			}
		}
	}

	return NULL;
}

