/***************************************************************************
    begin........: April 2015
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
   @file pathbuilder.c
   @brief Utility functions for building file paths.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <stdlib.h>
#include <assert.h>

#include "utils.h"

bool
path_builder_global_extensions(char *path, size_t path_len)
{
	const char *libdir = getenv("EFIND_LIBDIR");

	if(!libdir || !*libdir)
	{
		libdir = LIBDIR;
	}

	assert(libdir != NULL);

	return utils_path_join(libdir, "efind/extensions", path, path_len);
}

bool
path_builder_local_extensions(char *path, size_t path_len)
{
	const char *home = getenv("HOME");
	bool success = false;

	if(home && *home)
	{
		success = utils_path_join(home, ".efind/extensions", path, path_len);
	}

	return success;
}

bool
path_builder_blacklist(char *path, size_t path_len)
{
	const char *home = getenv("HOME");
	bool success = false;

	if(home && *home)
	{
		success = utils_path_join(home, ".efind/blacklist", path, path_len);
	}

	return success;
}

