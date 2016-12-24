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

#include "extension.h"
#include "extension-json.h"
#include "dl-ext-backend.h"
#include "utils.h"

ExtensionCallbackArgType
extension_callback_arg_type_try_parse(const char *string)
{
	ExtensionCallbackArgType type = EXTENSION_CALLBACK_ARG_TYPE_UNDEFINED;

	assert(string != NULL);

	if(string)
	{
		if(!strcmp(string, "integer"))
		{
			type = EXTENSION_CALLBACK_ARG_TYPE_INTEGER;
		}
		else if(!strcmp(string, "string"))
		{
			type = EXTENSION_CALLBACK_ARG_TYPE_STRING;
		}
	}

	return type;
}

void
extension_callback_free(ExtensionCallback *callback)
{
	if(callback)
	{
		free(callback->name);
		free(callback->types);
		free(callback);
	}
}

ExtensionCallback *
extension_callback_new(const char *name, uint32_t argc)
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
			cb->types = (ExtensionCallbackArgType *)utils_malloc(sizeof(ExtensionCallbackArgType) * argc);
			memset(cb->types, 0, sizeof(ExtensionModuleType));
		}
	}

	return cb;
}

ExtensionModuleType
extension_module_type_try_parse(const char *string)
{
	ExtensionModuleType type = EXTENSION_MODULE_TYPE_UNDEFINED;

	assert(string != NULL);

	if(string)
	{
		if(!strcmp(string, "shared-library"))
		{
			type = EXTENSION_MODULE_TYPE_SHARED_LIB;
		}
	}

	return type;
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

void
extension_module_free(ExtensionModule *module)
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

ExtensionModule *
extension_module_new(const char *filename, ExtensionModuleType type)
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
			module->callbacks = assoc_array_new(str_compare, free, (FreeFunc)extension_callback_free);
		}
		else
		{
			free(module);
			module = NULL;
		}
	}

	return module;
}

void
extension_module_set_callback(ExtensionModule *module, ExtensionCallback *callback)
{
	assert(module != NULL);
	assert(module->callbacks != NULL);
	assert(callback != NULL);

	assoc_array_set(module->callbacks, strdup(callback->name), callback, true);
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
		dir->modules = (AssocArray *)assoc_array_new(str_compare, free, (FreeFunc)extension_module_free);

		/* search for extension description files */
		struct dirent *entry;

		success = true;

		while(success && (entry = readdir(pdir)))
		{
			size_t len = strlen(entry->d_name);

			if(len >= 10 && !strcmp(entry->d_name + len - 9, ".ext.json"))
			{
				char filename[PATH_MAX];

				if(utils_path_join(dir->path, entry->d_name, filename, PATH_MAX))
				{
					success = extension_json_load_file(dir, filename, err);
				}
				else
				{
					utils_strdup_printf(err, "Path to extension description file exceeds maximum allowed path length.");
					success = false;
				}
			}
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
extension_dir_test_callback(ExtensionDir *dir, const char *name, uint32_t argc, ExtensionCallbackArgType *types)
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
extension_dir_invoke(ExtensionDir *dir, const char *name, const char *filename, struct stat *stbuf, uint32_t argc, void **argv, int *result)
{
	ExtensionCallbackStatus status = EXTENSION_CALLBACK_STATUS_NOT_FOUND;
	ExtensionModule *module;
	ExtensionCallback *cb;

	assert(dir != NULL);
	assert(name != NULL);
	assert(filename != NULL);
	assert(stbuf != NULL);

	if((module = _extension_dir_find_callback(dir, name, &cb)))
	{
		if(cb->argc == argc)
		{
			if(!module->backend.invoke(module->handle, name, filename, stbuf, argc, argv, result))
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

		args->types = (ExtensionCallbackArgType *)utils_malloc(sizeof(ExtensionCallbackArgType) * argc);
		memset(args->types, 0, sizeof(ExtensionCallbackArgType) * argc);
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
	args->types[offset] = EXTENSION_CALLBACK_ARG_TYPE_INTEGER;
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
	args->types[offset] = EXTENSION_CALLBACK_ARG_TYPE_STRING;
}

