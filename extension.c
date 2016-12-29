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
 * \file extension.c
 * \brief Plugable post-processing hooks.
 * \author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 * \version 0.1.0
 * \date 22. December 2016
 */

#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "extension.h"
#include "dl-ext-backend.h"
#include "utils.h"

static ExtensionCallback *
_extension_callback_new(const char *name, uint32_t argc)
{
	ExtensionCallback *cb = NULL;

	assert(name != NULL);

	if(name)
	{
		cb = (ExtensionCallback *)utils_malloc(sizeof(ExtensionCallback));
		memset(cb, 0, sizeof(ExtensionCallback));

		cb->name = strdup(name);
		cb->argc = argc;

		if(argc)
		{
			cb->types = (CallbackArgType *)utils_malloc(sizeof(CallbackArgType) * argc);
			memset(cb->types, 0, sizeof(CallbackArgType) * argc);
		}
	}

	return cb;
}

static void
_extension_callback_free(ExtensionCallback *callback)
{
	if(callback)
	{
		free(callback->name);
		free(callback->types);
		free(callback);
	}
}

static bool
_extension_module_set_backend_class(ExtensionBackendClass *cls, ExtensionModuleType type)
{
	bool success = true;

	assert(cls != NULL);

	switch(type)
	{
		case EXTENSION_MODULE_TYPE_SHARED_LIB:
			dl_extension_backend_get_class(cls);
			break;

		default:
			success = false;
	}

	return success;
}

ExtensionModule *
_extension_module_new(const char *filename, ExtensionModuleType type)
{
	ExtensionModule *module = NULL;

	if(filename && type != EXTENSION_MODULE_TYPE_UNDEFINED)
	{
		module = (ExtensionModule *)utils_malloc(sizeof(ExtensionModule));
		memset(module, 0, sizeof(ExtensionModule));

		if(_extension_module_set_backend_class(&module->backend, type))
		{
			module->filename = strdup(filename);
			module->type = type;
			module->callbacks = assoc_array_new(str_compare, free, (FreeFunc)_extension_callback_free);
		}
		else
		{
			free(module);
			module = NULL;
		}
	}

	return module;
}

static void
_extension_module_free(ExtensionModule *module)
{
	if(module)
	{
		if(module->callbacks)
		{
			assoc_array_destroy(module->callbacks);
		}

		if(module->handle)
		{
			module->backend.unload(module->handle);
		}

		free(module->filename);
		free(module);
	}
}

static void
_extension_dir_function_discovered(RegistrationCtx *ctx, const char *name, uint32_t argc, ...)
{
	ExtensionCallback *cb = NULL;

	assert(ctx != NULL);
	assert(name != NULL);
	assert(argc < 128);

	cb = _extension_callback_new(name, argc);

	if(cb)
	{
		va_list ap;
		bool success = true;

		va_start(ap, argc);

		for(uint32_t i = 0; i < argc && success; ++i)
		{
			int val = va_arg(ap, int);
			
			if(val == CALLBACK_ARG_TYPE_INTEGER || val == CALLBACK_ARG_TYPE_STRING)
			{
				cb->types[i] = val;
			}
			else
			{
				/* failure => free memory */
				_extension_callback_free(cb);
				success = false;
			}
		}

		va_end(ap);

		if(success)
		{
			assoc_array_set(((ExtensionModule *)ctx)->callbacks, strdup(cb->name), cb, true);
		}
	}
}

static bool
_extension_dir_import_module(ExtensionDir *dir, const char *filename, ExtensionModuleType type)
{
	ExtensionModule *module;
	bool success = false;

	assert(dir != NULL);
	assert(filename != NULL);

	module = _extension_module_new(filename, type);

	if(module)
	{
		/* load module */
		if((module->handle = module->backend.load(module->filename)))
		{
			assoc_array_set(dir->modules, strdup(module->filename), module, true);

			/* discover functions */
			module->backend.discover(module->handle, _extension_dir_function_discovered, (void *)module);

			success = true;
		}
	}

	return success;
}

ExtensionDir *
extension_dir_load(const char *path, char **err)
{
	ExtensionDir *dir = NULL;
	bool success = false;
	DIR *pdir;

	assert(path != NULL);

	/* try to open extension folder */
	if((pdir = opendir(path)))
	{
		/* create ExtensionDir instance & copy path */
		dir = (ExtensionDir *)utils_malloc(sizeof(ExtensionDir));
		memset(dir, 0, sizeof(ExtensionDir));

		dir->path = strdup(path);
		dir->modules = (AssocArray *)assoc_array_new(str_compare, free, (FreeFunc)_extension_module_free);

		/* search for extensions */
		struct dirent *entry;

		success = true;
		entry = readdir(pdir);

		while(success && entry)
		{
			size_t len = strlen(entry->d_name);

			if(len >= 3 && !strcmp(entry->d_name + len - 3, ".so"))
			{
				char filename[PATH_MAX];

				if(utils_path_join(dir->path, entry->d_name, filename, PATH_MAX))
				{
					if(!_extension_dir_import_module(dir, filename, EXTENSION_MODULE_TYPE_SHARED_LIB))
					{
						utils_strdup_printf(err, "Couldn't load extension \"%s\".", filename);
						success = false;
					}
				}
				else
				{
					utils_strdup_printf(err, "Path to extension description file exceeds maximum allowed path length.");
					success = false;
				}
			}

			entry = readdir(pdir);
		}

		closedir(pdir);
	}
	else
	{
		utils_strdup_printf(err, "Couldn't open directory \"%s\".", path);
	}

	if(!success && dir)
	{
		extension_dir_destroy(dir);
		dir = NULL;
	}

	return dir;
}

bool
extension_dir_register_module(ExtensionDir *dir, ExtensionModule *module)
{
	bool success = false;

	assert(dir != NULL);
	assert(dir->modules != NULL);
	assert(module != NULL);
	assert(module->filename != NULL);

	if((module->handle = module->backend.load(module->filename)))
	{
		assoc_array_set(dir->modules, strdup(module->filename), module, true);
		success = true;
	}

	return success;
}

void
extension_dir_destroy(ExtensionDir *dir)
{
	if(dir)
	{
		if(dir->modules)
		{
			assoc_array_destroy(dir->modules);
		}

		free(dir->path);
		free(dir);
	}
}

static ExtensionModule *
_extension_dir_find_callback(ExtensionDir *dir, const char *name, ExtensionCallback **cb)
{
	AssocArrayIter iter;

	assert(dir != NULL);
	assert(name != NULL);
	assert(cb != NULL);

	assoc_array_iter_init(dir->modules, &iter);

	while(assoc_array_iter_next(&iter))
	{
		ExtensionModule *module= (ExtensionModule *)assoc_array_iter_get_value(&iter);

		assert(module != NULL);

		*cb = assoc_array_lookup(module->callbacks, name);

		if(*cb)
		{
			return module;
		}
	}

	return NULL;
}

ExtensionCallbackStatus
extension_dir_test_callback(ExtensionDir *dir, const char *name, uint32_t argc, CallbackArgType *types)
{
	ExtensionCallback *cb;

	assert(dir != NULL);

	if(name != NULL)
	{
		if(_extension_dir_find_callback(dir, name, &cb))
		{
			if(cb->argc != argc)
			{
				return EXTENSION_CALLBACK_STATUS_INVALID_SIGNATURE;
			}

			for(uint32_t i = 0; i < argc; ++i)
			{
				if(types[i] != cb->types[i])
				{
					return EXTENSION_CALLBACK_STATUS_INVALID_SIGNATURE;
				}
			}

			return EXTENSION_CALLBACK_STATUS_OK;
		}
	}

	return EXTENSION_CALLBACK_STATUS_NOT_FOUND;
}

ExtensionCallbackStatus
extension_dir_invoke(ExtensionDir *dir, const char *name, const char *filename, uint32_t argc, void **argv, int *result)
{
	ExtensionCallbackStatus status = EXTENSION_CALLBACK_STATUS_NOT_FOUND;
	ExtensionModule *module;
	ExtensionCallback *cb;

	assert(dir != NULL);
	assert(name != NULL);
	assert(filename != NULL);

	if((module = _extension_dir_find_callback(dir, name, &cb)))
	{
		if(cb->argc == argc)
		{
			if(!module->backend.invoke(module->handle, name, filename, argc, argv, result))
			{
				status = EXTENSION_CALLBACK_STATUS_OK;
			}
		}
		else
		{
			status = EXTENSION_CALLBACK_STATUS_INVALID_SIGNATURE;
		}
	}

	return status;
}

ExtensionDir *
extension_dir_default(char **err)
{
	ExtensionDir *dir = NULL;
	const char *home = getenv("HOME");

	if(home)
	{
		char path[PATH_MAX];

		if(utils_path_join(home, ".efind/extensions", path, PATH_MAX))
		{
			dir = extension_dir_load(path, NULL);
		}
	}

	if(!dir)
	{
		dir = extension_dir_load("/etc/efind/extensions", err);
	}

	return dir;
}

ExtensionCallbackArgs *
extension_callback_args_new(uint32_t argc)
{
	ExtensionCallbackArgs *args;

	args = (ExtensionCallbackArgs *)utils_malloc(sizeof(ExtensionCallbackArgs));
	memset(args, 0, sizeof(ExtensionCallbackArgs));
	args->argc = argc;
	
	if(args->argc > 0)
	{
		args->argv = (void **)utils_malloc(sizeof(void *) * argc);
		memset(args->argv, 0, sizeof(void *) * argc);

		args->types = (CallbackArgType *)utils_malloc(sizeof(CallbackArgType) * argc);
		memset(args->types, 0, sizeof(CallbackArgType) * argc);
	}

	return args;
}

void
extension_callback_args_free(ExtensionCallbackArgs *args)
{
	if(args && args->argc > 0)
	{
		assert(args->argv != NULL);
		assert(args->types != NULL);

		free(args->types);

		for(uint32_t i = 0; i < args->argc; ++i)
		{
			free(args->argv[i]);
		}

		free(args->argv);
	}

	free(args);
}

void
extension_callback_args_set_integer(ExtensionCallbackArgs *args, uint32_t offset, int32_t value)
{
	assert(args);
	assert(args->argc > offset);

	if(args->argv[offset])
	{
		free(args->argv[offset]);
	}

	args->argv[offset] = utils_malloc(sizeof(int32_t));
	*((int32_t *)args->argv[offset]) = value;
	args->types[offset] = CALLBACK_ARG_TYPE_INTEGER;
}

void
extension_callback_args_set_string(ExtensionCallbackArgs *args, uint32_t offset, const char *string)
{
	assert(args);
	assert(args->argc > offset);

	if(args->argv[offset])
	{
		free(args->argv[offset]);
	}

	args->argv[offset] = strdup(string);
	args->types[offset] = CALLBACK_ARG_TYPE_STRING;
}

