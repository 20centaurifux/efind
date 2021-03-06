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
   @file dl-ext-backend.c
   @brief libdl filter function backend.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <dlfcn.h>
#include <assert.h>

#include "dl-ext-backend.h"
#include "extension-interface.h"
#include "log.h"
#include "utils.h"
#include "gettext.h"

static void *
_dl_ext_backend_load(const char *filename, RegisterExtension fn, RegistrationCtx *ctx)
{
	void *handle;

	assert(filename != NULL);
	assert(fn != NULL);

	handle = dlopen(filename, RTLD_LAZY);

	if(handle)
	{
		void (*registration)(RegistrationCtx *ctx, RegisterExtension register_fn);

		registration = dlsym(handle, "registration");

		if(registration)
		{
			registration(ctx, fn);
		}
		else
		{
			WARNINGF("extension", "No registration() function found in file `%s'.", filename);

			dlclose(handle);
			handle = NULL;
		}
	}

	return handle;
}

static void
_dl_ext_discover(void *handle, RegisterCallback fn, RegistrationCtx *ctx)
{
	void (*discover)(RegistrationCtx *ctx, RegisterCallback register_fn);
	
	assert(handle != NULL);
	assert(fn != NULL);

	discover = dlsym(handle, "discover");

	if(discover)
	{
		discover(ctx, fn);
	}
	else
	{
		DEBUG("extension", "discover() function not found.");
	}
}

static int
_dl_ext_backend_invoke(void *handle, const char *name, const char *filename, uint32_t argc, void **argv, int *result)
{
	int (*fn)(const char *filename, int argc, void **argv);
	int success = -1;

	assert(handle != NULL);
	assert(name != NULL);
	assert(filename != NULL);
	assert(result != NULL);

	fn = dlsym(handle, name);

	if(fn)
	{
		*result = fn(filename, argc, argv);
		success = 0;
	}
	else
	{
		DEBUGF("extension", "dlsym() failed, symbol `%s' not found.", name);

		fprintf(stderr, _("Function `%s' not found.\n"), name);
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
	assert(cls != NULL);

	cls->load = _dl_ext_backend_load;
	cls->discover = _dl_ext_discover;
	cls->invoke = _dl_ext_backend_invoke;
	cls->unload = _dl_ext_backend_unload;
}

