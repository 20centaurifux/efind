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

#include <jansson.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <bsd/string.h>

#include "extension.h"
#include "dl-ext-backend.h"
#include "utils.h"

/*! @cond INTERNAL */
#define MAX_CALLBACK_NAME 64
#define MAX_CALLBACK_ARGS 16

typedef struct
{
	char name[MAX_CALLBACK_NAME];
	uint32_t argc;
	ExtensionCallbackArgType types[MAX_CALLBACK_ARGS];
} ExtensionCallback;

typedef enum
{
	EXTENSION_MODULE_TYPE_UNDEFINED,
	EXTENSION_MODULE_TYPE_SHARED_LIB
} ExtensionModuleType;

typedef struct
{
	char filename[PATH_MAX];
	ExtensionModuleType type;
	ExtensionBackendClass backend;
	void *handle;
	AssocArray *callbacks;
} ExtensionModule;
/*! @endcond */

static ExtensionCallbackArgType
_extension_callback_arg_type_try_parse(const char *string)
{
	ExtensionCallbackArgType type = EXTENSION_CALLBACK_ARG_TYPE_UNDEFINED;

	if(!strcmp(string, "integer"))
	{
		type = EXTENSION_CALLBACK_ARG_TYPE_INTEGER;
	}
	else if(!strcmp(string, "string"))
	{
		type = EXTENSION_CALLBACK_ARG_TYPE_STRING;
	}

	return type;
}

static void
_extension_callback_destroy(ExtensionCallback *callback)
{
	free(callback);
}

static ExtensionCallback *
_extension_callback_from_json(void *ptr, char **err)
{
	ExtensionCallback *callback = NULL;

	assert(ptr);

	/* get callback name */
	const char *cb_name = json_object_iter_key(ptr);

	if(!cb_name)
	{
		utils_strdup_printf(err, "Couldn't determine callback name.");
		goto error;
	}

	callback = (ExtensionCallback *)utils_malloc(sizeof(ExtensionCallback));
	memset(callback, 0, sizeof(ExtensionCallback));

	if(strlcpy(callback->name, cb_name, MAX_CALLBACK_NAME) >= MAX_CALLBACK_NAME)
	{
		utils_strdup_printf(err, "Callback name \"%s\" exceeds maximum length of %d characters.", callback->name, MAX_CALLBACK_NAME);
		goto error;
	}

	json_t *desc = json_object_iter_value(ptr);

	if(json_is_object(desc))
	{
		/* get callback signature */
		json_t *child = json_object_get(desc, "args");

		if(child && json_is_array(child))
		{
			size_t len = json_array_size(child);

			if(len > MAX_CALLBACK_ARGS)
			{
				utils_strdup_printf(err, "Callback \"%s\" has too many arguments.", callback->name);
				goto error;
			}

			for(uint32_t i = 0; i < len; ++i)
			{
				json_t *type_name = json_array_get(child, i);

				if(json_is_string(type_name))
				{
					callback->types[callback->argc] =
						_extension_callback_arg_type_try_parse(json_string_value(type_name));

					if(callback->types[callback->argc] == EXTENSION_CALLBACK_ARG_TYPE_UNDEFINED)
					{
						utils_strdup_printf(err,
						                    "Callback \"%s\" has argument with unknown data type \"%s\".",
						                    callback->name,
						                    json_string_value(type_name));
						goto error;
					}
				}
				else
				{
					utils_strdup_printf(err, "Callback argument data type must be a string value.");
					goto error;
				}

				++callback->argc;
			}
		}
	}
	else if(!json_is_null(desc))
	{
		utils_strdup_printf(err, "Description of callback \"%s\" is invalid.", callback->name);
		goto error;
	}

	return callback;

error:
	_extension_callback_destroy(callback);

	return NULL;
}

static ExtensionModuleType
_extension_module_type_try_parse(const char *string)
{
	ExtensionModuleType type = EXTENSION_MODULE_TYPE_UNDEFINED;

	if(!strcmp(string, "shared-library"))
	{
		type = EXTENSION_MODULE_TYPE_SHARED_LIB;
	}

	return type;
}

static bool
_extension_module_set_backend(ExtensionModuleType type, ExtensionBackendClass *cls)
{
	bool success = true;

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

		free(module);
	}
}

static ExtensionModule *
_extension_module_from_json(const char *path, void *ptr, char **err)
{
	ExtensionModule *module = NULL;

	assert(path != NULL);
	assert(ptr != NULL);

	/* build extension filename */
	const char *module_name = json_object_iter_key(ptr);

	if(!module_name)
	{
		utils_strdup_printf(err, "Found module without module name.");
		goto error;
	}

	module = (ExtensionModule *)utils_malloc(sizeof(ExtensionModule));
	memset(module, 0, sizeof(ExtensionModule));

	if(!utils_path_join(path, module_name, module->filename, PATH_MAX))
	{
		utils_strdup_printf(err, "Path to \"extensions.js\" exceeds maximum allowed path length.");
		goto error;
	}

	/* test if file does exist */
	struct stat stbuf;

	if(stat(module->filename, &stbuf))
	{
		utils_strdup_printf(err, "Couldn't find \"%s\"", module->filename);
		goto error;
	}

	if((stbuf.st_mode & S_IFMT) != S_IFREG)
	{
		utils_strdup_printf(err, "\"%s\" is not a regular file.", module->filename);
		goto error;
	}

	/* get module type */
	json_t *child = json_object_iter_value(ptr);

	if(!child)
	{
		utils_strdup_printf(err, "Module \"%s\" has no description.", module_name);
		goto error;
	}

	json_t *grandchild = json_object_get(child, "type");

	if(!grandchild || !json_is_string(grandchild))
	{
		utils_strdup_printf(err, "Module \"%s\" has no valid type description.", module_name);
		goto error;
	}

	module->type = _extension_module_type_try_parse(json_string_value(grandchild));

	if(module->type == EXTENSION_MODULE_TYPE_UNDEFINED)
	{
		utils_strdup_printf(err, "Module \"%s\" has an unknown type description.", module_name);
		goto error;
	}

	/* initialize backend */
	if(!_extension_module_set_backend(module->type, &module->backend))
	{
		utils_strdup_printf(err, "Couldn't initialize backend for extension \"%s\".", module_name);
		goto error;
	}

	if(!(module->handle = module->backend.load(module->filename)))
	{
		utils_strdup_printf(err, "Couldn't load extension (\"%s\").", module_name);
		goto error;
	}

	/* get callbacks */
	grandchild = json_object_get(child, "export");

	if(!grandchild || !json_is_object(grandchild))
	{
		utils_strdup_printf(err, "Module \"%s\" has no exported callbacks.", module_name);
		goto error;
	}

	module->callbacks = assoc_array_new(str_compare, free, (FreeFunc)_extension_callback_destroy);

	void *iter = json_object_iter(grandchild);

	while(iter)
	{
		ExtensionCallback *callback = _extension_callback_from_json(iter, err);

		if(!callback)
		{
			goto error;
		}

		assoc_array_set(module->callbacks, strdup(callback->name), callback, true);
		iter = json_object_iter_next(grandchild, iter);
	}

	return module;

error:
	_extension_module_free(module);

	return NULL;
}

static bool
_extension_dir_load_json_file(ExtensionDir *dir, const char *filename, char **err)
{
	bool success = false;
	json_t *json;
	json_error_t json_err;

	assert(dir != NULL);
	assert(filename != NULL);

	json = json_load_file(filename, 0, &json_err);

	if(!json || !json_is_object(json))
	{
		if(json_err.text)
		{
			utils_strdup_printf(err, "Couldn't load \"extensions.js\": %s", json_err.text);
		}
		else
		{
			utils_strdup_printf(err, "Couldn't load \"extensions.js\"");
		}
		
		goto out;
	}

	dir->modules = (AssocArray *)assoc_array_new(str_compare, free, (FreeFunc)_extension_module_free);

	void *iter = json_object_iter(json);

	while(iter)
	{
		ExtensionModule *module = _extension_module_from_json(dir->path, iter, err);

		if(!module)
		{
			goto out;
		}

		assoc_array_set(dir->modules, strdup(module->filename), module, true);
		iter = json_object_iter_next(json, iter);
	}

	success = true;

out:
	json_decref(json);

	return success;
}

ExtensionDir *
extension_dir_load(const char *path, char **err)
{
	ExtensionDir *dir = NULL;

	assert(path != NULL);

	/* test if directory does exist */
	struct stat stbuf;

	if(stat(path, &stbuf))
	{
		utils_strdup_printf(err, "Couldn't stat \"%s\".", path);
		goto error;
	}

	if((stbuf.st_mode & S_IFMT) != S_IFDIR)
	{
		utils_strdup_printf(err, "\"%s\" is not a directory.", path);
		goto error;
	}

	/* create ExtensionDir instance & copy path */
	dir = (ExtensionDir *)utils_malloc(sizeof(ExtensionDir));
	memset(dir, 0, sizeof(ExtensionDir));

	if(strlcpy(dir->path, path, PATH_MAX) >= PATH_MAX)
	{
		utils_strdup_printf(err, "\"%s\" exceeds the maximum allowed path length.", path);
		goto error;
	}

	/* load "extension.js" file */
	char filename[PATH_MAX];

	if(!utils_path_join(dir->path, "extensions.js", filename, PATH_MAX))
	{
		utils_strdup_printf(err, "Path to extension description file exceeds maximum allowed path length.");
		goto error;
	}

	if(!_extension_dir_load_json_file(dir, filename, err))
	{
		goto error;
	}

	return dir;

error:
	if(dir)
	{
		extension_dir_destroy(dir);
	}

	return NULL;
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

		free(dir);
	}
}

static ExtensionModule *
_extension_dir_find_callback(ExtensionDir *dir, const char *name, ExtensionCallback **cb)
{
	AssocArrayIter iter;

	assert(dir != NULL);
	assert(name != NULL);

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

	if(name == NULL)
	{
		return EXTENSION_CALLBACK_NOT_FOUND;
	}

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

	return EXTENSION_CALLBACK_NOT_FOUND;
}

ExtensionCallbackStatus
extension_dir_invoke(ExtensionDir *dir, const char *name, const char *filename, struct stat *stbuf, uint32_t argc, void **argv, int *result)
{
	ExtensionCallbackStatus status = EXTENSION_CALLBACK_NOT_FOUND;
	ExtensionModule *module;
	ExtensionCallback *cb;

	assert(dir != NULL);
	assert(name != NULL);
	assert(filename != NULL);

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

