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

static bool
_path_build_local(const char *filename, char *path, size_t path_len)
{
	bool success = false;

	assert(filename != NULL);
	assert(path != NULL);

	const char *home = getenv("HOME");

	if(home && *home)
	{
		success = utils_path_join(home, filename, path, path_len);
	}

	return success;
}

bool
path_builder_global_ini(char *path, size_t path_len)
{
	assert(path != NULL);

	return utils_path_join(SYSCONFDIR, "efind/config", path, path_len);
}

bool
path_builder_local_ini(char *path, size_t path_len)
{
	assert(path != NULL);

	return _path_build_local(".efind/config", path, path_len);
}

bool
path_builder_global_extensions(char *path, size_t path_len)
{
	assert(path != NULL);

	const char *libdir = getenv("EFIND_LIBDIR");

	if(!libdir || !*libdir)
	{
		libdir = LIBDIR;
	}

	return utils_path_join(libdir, "efind/extensions", path, path_len);
}

bool
path_builder_local_extensions(char *path, size_t path_len)
{
	assert(path != NULL);

	return _path_build_local(".efind/extensions", path, path_len);
}

bool
path_builder_blacklist(char *path, size_t path_len)
{
	assert(path != NULL);

	return _path_build_local(".efind/blacklist", path, path_len);
}

