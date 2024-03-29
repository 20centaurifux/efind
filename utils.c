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
#include <limits.h>
#include <ctype.h>
#include <stdint.h>

#include "utils.h"
#include "log.h"
#include "gettext.h"

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
	assert(ptr != NULL);
	assert(size > 0);

	if(!(ptr = realloc(ptr, size)))
	{
		perror("realloc()");
		abort();
	}

	return ptr;
}

void *
utils_realloc_n(void *ptr, size_t nmemb, size_t size)
{
	assert(nmemb > 0);
	assert(size > 0);

	if(SIZE_MAX / size < nmemb)
	{
		fprintf(stderr, _("Couldn't resize array due to integer overflow.\n"));
		abort();
	}

	return utils_realloc(ptr, nmemb * size);
}

void *
utils_calloc(size_t nmemb, size_t size)
{
	void *ptr;

	assert(nmemb > 0);
	assert(size > 0);

	if(!(ptr = calloc(nmemb, size)))
	{
		perror("calloc()");
		abort();
	}

	return ptr;
}

char *
utils_strdup(const char *str)
{
	char *ptr = NULL;

	assert(str != NULL);

	if(str)
	{
		ptr = strdup(str);

		if(!ptr)
		{
			fprintf(stderr, "strdup() failed.\n");
			abort();
		}
	}

	return ptr;
}

void
utils_copy_string(const char *src, char **dst)
{
	assert(src != NULL);
	assert(dst != NULL);

	if(*dst)
	{
		size_t old_len = strlen(*dst);
		size_t new_len = strlen(src);

		if(new_len == SIZE_MAX)
		{
			fprintf(stderr, _("Couldn't allocate memory.\n"));
			abort();
		}

		if(new_len > old_len)
		{
			*dst = utils_realloc(*dst, sizeof(char *) * (new_len + 1));
		}

		strcpy(*dst, src);
	}
	else
	{
		*dst = utils_strdup(src);
	}
}

size_t
utils_strlcat(char *dst, const char *src, size_t size)
{
	size_t ret = 0;

	assert(dst != NULL);

	char *d = dst;
	const char *s = src;

	if(src)
	{
		size_t n = size;
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

	size_t start = 0;
	size_t end = strlen(str);

	if(!end)
	{
		return 0;
	}

	while(str[start] && isspace(str[start]))
	{
		++start;
	}

	while(end > start && isspace(str[end - 1]))
	{
		--end;
	}

	const size_t len = end - start;

	if(len)
	{
		memmove(str, str + start, len);
	}

	str[len] = '\0';

	return len;
}

bool
utils_startswith(const char *str, const char *prefix)
{
	bool success = false;

	assert(str != NULL);
	assert(prefix != NULL);

	if(str && prefix)
	{
		size_t n = strlen(prefix);

		if(strlen(str) >= n)
		{
			success = !strncmp(str, prefix, n);
		}
	}

	return success;
}

bool
utils_int_add_checkoverflow(int a, int b, int *dst)
{
	bool overflow = true;

	assert(a >= 0);
	assert(b >= 0);

	#if __GNUC__ > 4
	overflow = __builtin_add_overflow(a, b, dst);
	#else
	if(INT_MAX - b >= a)
	{
		*dst = a + b;
		overflow = false;
	}
	#endif

	return overflow;
}

char *
utils_whereis(const char *name)
{
	char *exe = NULL;

	assert(name != NULL);

	char *rest = getenv("PATH");

	if(!rest || !*rest)
	{
		return NULL;
	}

	char *token;

	while(!exe && (token = strtok_r(rest, ":", &rest)))
	{
		size_t len = strlen(token);

		if(!len)
		{
			continue;
		}

		char path[PATH_MAX];

		if(utils_path_join(token, name, path, PATH_MAX))
		{
			struct stat sb;

			if(!stat(path, &sb))
			{
				if((sb.st_mode & S_IFMT) == S_IFREG && (sb.st_mode & S_IXUSR || sb.st_mode & S_IXGRP))
				{
					exe = utils_strdup(path);
				}
			}
		}
		else
		{
			ERROR("misc", "String truncated.");
		}
	}

	return exe;
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

		result = len > 0 && len < max_len;
	}

	return result;
}

bool
utils_parse_integer(const char *value, long int min, long int max, long int *dst)
{
	bool success = false;

	assert(value != NULL);
	assert(dst != NULL);

	if(value)
	{
		char *tail = NULL;
		long int v = strtol(value, &tail, 10);

		if(tail && *tail == '\0' && v >= min && v <= max)
		{
			*dst = v;
			success = true;
		}
	}

	return success;
}

bool
utils_parse_bool(const char *value, bool *dst)
{
	bool success = false;

	assert(value != NULL);
	assert(dst != NULL);

	if(value)
	{
		success = true;

		if(!strcmp(value, "yes"))
		{
			*dst = true;
		}
		else if(!strcmp(value, "no"))
		{
			*dst = false;
		}
		else
		{
			success = false;
		}
	}

	return success;
}

