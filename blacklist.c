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
   @file blacklist.c
   @brief List containing blacklisted files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
/*! @cond INTERNAL */
#define _GNU_SOURCE
/*! @endcond */

#include "blacklist.h"
#include "log.h"
#include "utils.h"
#include "pathbuilder.h"

#include <stdio.h>
#include <glob.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>

Blacklist *
blacklist_new(void)
{
	return list_new(str_compare, free, NULL);
}

void
blacklist_destroy(Blacklist *blacklist)
{
	assert(blacklist != NULL);

	list_destroy(blacklist);
}

size_t
blacklist_glob(Blacklist *blacklist, const char *pattern)
{
	glob_t g;
	size_t count = 0;

	assert(blacklist != NULL);
	assert(pattern != NULL);

	memset(&g, 0, sizeof(glob_t));

	DEBUGF("blacklist", "Adding pattern to blacklist: %s", pattern);

	int rc = glob(pattern, GLOB_TILDE, NULL, &g);

	if(!rc)
	{
		for(size_t i = 0; i < g.gl_pathc; ++i)
		{
			if(!list_contains(blacklist, g.gl_pathv[i]))
			{
				TRACEF("blacklist", "Appending file to blacklist: %s", g.gl_pathv[i]);
				list_append(blacklist, utils_strdup(g.gl_pathv[i]));
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
	assert(blacklist != NULL);
	assert(filename != NULL);

	return list_contains(blacklist, (void *)filename);
}

size_t
blacklist_load(Blacklist *blacklist, const char *filename)
{
	FILE *fp;
	size_t count = 0;

	assert(blacklist != NULL);
	assert(filename != NULL);

	DEBUGF("blacklist", "Loading file: %s", filename);

	if((fp = fopen(filename, "r")))
	{
		char *pattern = (char *)utils_malloc(64);
		size_t bytes = 64;
		ssize_t result;

		result = getline(&pattern, &bytes, fp);

		while(result > 0 && count != SIZE_MAX)
		{
			size_t len = utils_trim(pattern);

			if(len && *pattern != '#')
			{
				size_t pathc = blacklist_glob(blacklist, pattern);

				if(SIZE_MAX - count >= pathc)
				{
					count += pathc;
				}
				else
				{
					WARNING("blacklist", "Integer overflow.");
					count = SIZE_MAX;
				}
			}

			result = getline(&pattern, &bytes, fp);
		}

		free(pattern);
		fclose(fp);
	}
	else
	{
		DEBUGF("blacklist", "File not found: %s", filename);
	}

	return count;
}

size_t
blacklist_load_default(Blacklist *blacklist)
{
	size_t count = 0;
	char path[PATH_MAX];

	assert(blacklist != NULL);

	DEBUG("blacklist", "Loading default blacklist.");

	if(path_builder_blacklist(path, PATH_MAX))
	{
		count = blacklist_load(blacklist, path);
	}
	else
	{
		WARNING("blacklist", "Couldn't build path to blacklist.");
	}

	return count;
}

