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
#include <bsd/string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>

#include "utils.h"

void *
utils_malloc(size_t size)
{
	void *ptr;

	assert(size > 0);

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
	assert(size > 0);

	if(!(ptr = realloc(ptr, size)))
	{
		perror("realloc()");
		abort();
	}

	return ptr;
}

char *
utils_whereis(const char *name)
{
	char *rest;
	char *token;
	char path[512];
	int written;
	struct stat sb;

	assert(name != NULL);

	rest = getenv("PATH");

	/* test if PATH is empty */
	if(!rest || !rest[0])
	{
		return NULL;
	}

	while((token = strtok_r(rest, ":", &rest)))
	{
		/* build absolute path to executable file */
		size_t len = strlen(token);

		if(!len)
		{
			continue;
		}

		if(token[len - 1] == '/')
		{
			written = snprintf(path, 512, "%s%s", token, name);
		}
		else
		{
			written = snprintf(path, 512, "%s/%s", token, name);
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

size_t
utils_printf_loc(const Node *node, char *buf, size_t size, const char *format, ...)
{
	va_list ap;
	const YYLTYPE *locp;
	char tmp[512];
	size_t ret;

	assert(node != NULL);
	assert(buf != NULL);
	assert(size > 0);
	assert(format != NULL);

	locp = &node->loc;

	memset(buf, 0, size);
	memset(tmp, 0, sizeof(tmp));

	if(locp->first_line == locp->last_line)
	{
		snprintf(tmp, sizeof(tmp), "line: %d, ", locp->first_line);
	}
	else
	{
		snprintf(tmp, sizeof(tmp), "line: %d-%d, ", locp->first_line, locp->last_line);
	}

	if((ret = strlcat(buf, tmp, size)) > size)
	{
		goto out;
	}

	if(locp->first_column == locp->last_column)
	{
		snprintf(tmp, 32, "column: %d: ", locp->first_column);
	}
	else
	{
		snprintf(tmp, 32, "column: %d-%d: ", locp->first_column, locp->last_column);
	}

	if((ret = strlcat(buf, tmp, size)) > size)
	{
		goto out;
	}

	va_start(ap, format);

	if(vsnprintf(tmp, sizeof(tmp), format, ap) < sizeof(tmp))
	{
		ret = strlcat(buf, tmp, size);
	}

	va_end(ap);

	out:
		return ret;
}

bool
utils_path_join(const char *dir, const char *filename, char *dst, size_t max_len)
{
	bool result = false;

	assert(dir != NULL);
	assert(filename != NULL);

	memset(dst, 0, max_len);

	size_t len = strlen(dir);

	if(len < max_len)
	{
		if(dir[len - 1] == '/')
		{
			len = snprintf(dst, max_len, "%s%s", dir, filename);
		}
		else
		{
			len = snprintf(dst, max_len, "%s/%s", dir, filename);
		}

		result = len <= max_len;
	}

	return result;
}

void
utils_strdup_printf(char **dst, const char *fmt, ...)
{
	char str[4096];
	va_list ap;

	if(!dst || !fmt || *dst)
	{
		return;
	}

	*dst = NULL;

	va_start(ap, fmt);

	if(vsnprintf(str, sizeof(str), fmt, ap) < 4096)
	{
		*dst = strdup(str);
	}
	
	va_end(ap);
}

