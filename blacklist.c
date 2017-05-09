/***************************************************************************
    begin........: May 2015
    copyright....: Sebastian Fedrau
    email........: sebastian.fedrau@gmail.com
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License v3 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License v3 for more details.
 ***************************************************************************/
/**
 * @file blacklist.c
 * @brief List containing blacklisted files.
 * @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#define _GNU_SOURCE

#include "blacklist.h"
#include "utils.h"

#include <stdio.h>
#include <glob.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

Blacklist *
blacklist_new(void)
{
	return list_new(str_equal, free, NULL);	
}

void
blacklist_destroy(Blacklist *list)
{
	list_destroy(list);
}

size_t
blacklist_glob(Blacklist *blacklist, const char *pattern)
{
	glob_t g;
	size_t count = 0;

	assert(blacklist != NULL);
	assert(pattern != NULL);

	memset(&g, 0, sizeof(glob_t));

	int rc = glob(pattern, GLOB_TILDE, NULL, &g);

	if(!rc)
	{
		for(size_t i = 0; i < g.gl_pathc; i++)
		{
			if(!list_contains(blacklist, g.gl_pathv[i]))
			{
				list_append(blacklist, strdup(g.gl_pathv[i]));
			}
		}

		count = g.gl_pathc;
	}

	globfree(&g);

	return count;
}

bool
blacklist_matches(Blacklist *blacklist, const char *filename)
{
	return list_contains(blacklist, (void *)filename);
}

size_t
blacklist_load(Blacklist *blacklist, const char *filename)
{
	size_t count = 0;

	assert(blacklist != NULL);
	assert(filename != NULL);

	FILE *fp;

	if((fp = fopen(filename, "r")))
	{
		char *pattern = (char *)utils_malloc(64);
		size_t bytes = 64;
		ssize_t result;

		result = getline(&pattern, &bytes, fp);

		while(result > 0)
		{
			size_t len = utils_trim(pattern);

			if(len && *pattern != '#')
			{
				count += blacklist_glob(blacklist, pattern);
			}

			result = getline(&pattern, &bytes, fp);
		}

		free(pattern);
		fclose(fp);
	}

	return count;
}

size_t
blacklist_load_default(Blacklist *blacklist)
{
	size_t count = 0;

	assert(blacklist != NULL);

	const char *home = getenv("HOME");

	if(home)
	{
		char path[PATH_MAX];

		if(utils_path_join(home, ".efind/blacklist", path, PATH_MAX))
		{
			count = blacklist_load(blacklist, path);
		}
	}

	return count;
}

