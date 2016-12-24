/***************************************************************************
    begin........: May 2012
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
/*!
 * \file dl-ext-backend.c
 * \brief Plugable post-processing hooks backend using libdl.
 * \author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 * \version 0.1.0
 * \date 23. December 2016
 */

#include <dlfcn.h>
#include <assert.h>

#include "dl-ext-backend.h"
#include "utils.h"

static void *
_dl_ext_backend_load(const char *filename)
{
	assert(filename != NULL);

	return dlopen(filename, RTLD_LAZY);
}

static int
_dl_ext_backend_invoke(void *handle, const char *name, const char *filename, struct stat *stbuf, uint32_t argc, void **argv, int *result)
{
	assert(handle != NULL);
	assert(name != NULL);
	assert(filename != NULL);
	assert(result != NULL);

	int (*fn)(const char *filename, struct stat *stbuf, int argc, void **argv);
	int success = -1;

	fn = dlsym(handle, name);

	if(fn)
	{
		*result = fn(filename, stbuf, argc, argv);
		success = 0;
	}

	return success;
}

static void
_dl_ext_backend_unload(void *handle)
{
	if(handle != NULL)
	{
		dlclose(handle);
	}
}

void
dl_extension_backend_get_class(ExtensionBackendClass *cls)
{
	cls->load = _dl_ext_backend_load;
	cls->invoke = _dl_ext_backend_invoke;
	cls->unload = _dl_ext_backend_unload;
}

