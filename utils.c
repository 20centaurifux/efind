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
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>

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

size_t
utils_strlcat(char *dst, const char *src, size_t size)
{
	char *d = dst;
	const char *s = src;
	size_t n = size;
	size_t ret = 0;

	assert(dst != NULL);

	if(src)
	{
		size_t dlen;

		while(n-- != 0 && *d != '\0')
		{
			d++;
		}
		
		dlen = d - dst;
		n = size - dlen;

		if(n)
		{
			while(*s != '\0')
			{
				if(n != 1)
				{
					*d++ = *s;
					n--;
				}

				s++;
			}

			*d = '\0';

			ret = dlen + (s - src);
		}
		else
		{
			ret = dlen + strlen(s);
		}
	}

	return ret;
}

size_t
utils_trim(char *str)
{
	if(!str)
	{
		return 0;
	}

	size_t start = 0, end = strlen(str);

	if(!end)
	{
		return 0;
	}

	while(str[start] && isspace(str[start]))
	{
		start++;
	}

	while(end > start && isspace(str[end - 1]))
	{
		end--;
	}

	size_t len = end - start;

	if(len)
	{
		memmove(str, str + start, len);
	}

	str[len] = '\0';

	return len;
}

char *
utils_whereis(const char *name)
{
	char *rest;
	char *token;
	char path[PATH_MAX];
	struct stat sb;
	char *exe = NULL;

	assert(name != NULL);

	rest = getenv("PATH");

	/* test if PATH is empty */
	if(!rest || !rest[0])
	{
		return NULL;
	}

	while(!exe && (token = strtok_r(rest, ":", &rest)))
	{
		/* build absolute path to executable file */
		size_t len = strlen(token);

		if(!len)
		{
			continue;
		}

		if(utils_path_join(token, name, path, PATH_MAX))
		{
			/* test if file does exist & can be executed by user */
			if(!stat(path, &sb))
			{
				if((sb.st_mode & S_IFMT) == S_IFREG && (sb.st_mode & S_IXUSR || sb.st_mode & S_IXGRP))
				{
					exe = strdup(path);
				}
			}
		}
		else
		{
			fprintf(stderr, "%s: string truncation\n", __func__);
		}
	}

	return exe;
}

size_t
utils_printf_loc(const Node *node, char *buf, size_t size, const char *format, ...)
{
	va_list ap;
	const YYLTYPE *locp;
	char tmp[512];
	size_t ret = -1;

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

	if(utils_strlcat(buf, tmp, size) >= size)
	{
		goto out;
	}

	if(locp->first_column == locp->last_column)
	{
		snprintf(tmp, sizeof(tmp), "column: %d: ", locp->first_column);
	}
	else
	{
		snprintf(tmp, sizeof(tmp), "column: %d-%d: ", locp->first_column, locp->last_column);
	}

	if(utils_strlcat(buf, tmp, size) >= size)
	{
		goto out;
	}

	va_start(ap, format);

	if(vsnprintf(tmp, sizeof(tmp), format, ap) < sizeof(tmp))
	{
		if((ret = utils_strlcat(buf, tmp, size)) >= size)
		{
			ret = -1;
		}
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
	if(dst && fmt && *dst)
	{
		char str[4096];
		va_list ap;

		*dst = NULL;

		va_start(ap, fmt);

		if(vsnprintf(str, sizeof(str), fmt, ap) < 4096)
		{
			*dst = strdup(str);
		}
		
		va_end(ap);
	}
}

