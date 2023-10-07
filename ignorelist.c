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
   @file ignorelist.c
   @brief List containing ignored files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
/*! @cond INTERNAL */
#define _GNU_SOURCE
/*! @endcond */

#include <stdio.h>
#include <glob.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#include "ignorelist.h"
#include "log.h"
#include "utils.h"
#include "pathbuilder.h"
#include "gettext.h"

Ignorelist *
ignorelist_new(void)
{
	return list_new(str_compare, free, NULL);
}

void
ignorelist_destroy(Ignorelist *ignorelist)
{
	assert(ignorelist != NULL);

	list_destroy(ignorelist);
}

size_t
ignorelist_glob(Ignorelist *ignorelist, const char *pattern)
{
	glob_t g;
	size_t count = 0;

	assert(ignorelist != NULL);
	assert(pattern != NULL);

	memset(&g, 0, sizeof(glob_t));

	DEBUGF("ignorelist", "Adding pattern to ignore-list: %s", pattern);

	int rc = glob(pattern, GLOB_TILDE, NULL, &g);

	if(!rc)
	{
		for(size_t i = 0; i < g.gl_pathc; ++i)
		{
			if(!list_contains(ignorelist, g.gl_pathv[i]))
			{
				TRACEF("ignorelist", "Appending file to ignore-list: %s", g.gl_pathv[i]);
				list_append(ignorelist, utils_strdup(g.gl_pathv[i]));
			}
		}

		count = g.gl_pathc;
	}

	globfree(&g);

	return count;
}

bool
ignorelist_matches(const Ignorelist *ignorelist, const char *filename)
{
	assert(ignorelist != NULL);
	assert(filename != NULL);

	return list_contains(ignorelist, (void *)filename);
}

size_t
ignorelist_load(Ignorelist *ignorelist, const char *filename)
{
	FILE *fp;
	size_t count = 0;

	assert(ignorelist != NULL);
	assert(filename != NULL);

	DEBUGF("ignorelist", "Loading file: %s", filename);

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
				size_t pathc = ignorelist_glob(ignorelist, pattern);

				if(SIZE_MAX - count >= pathc)
				{
					count += pathc;
				}
				else
				{
					WARNING("ignorelist", "Integer overflow.");

					fprintf(stderr, _("Couldn't allocate memory.\n"));

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
		DEBUGF("ignorelist", "File not found: %s", filename);
	}

	return count;
}

size_t
ignorelist_load_default(Ignorelist *ignorelist)
{
	size_t count = 0;
	char path[PATH_MAX];

	assert(ignorelist != NULL);

	DEBUG("ignorelist", "Loading default ignore-list.");

	if(path_builder_ignorelist(path, PATH_MAX))
	{
		count = ignorelist_load(ignorelist, path);
	}
	else
	{
		WARNING("ignorelist", "Couldn't build path to ignore-list.");
	}

	return count;
}

