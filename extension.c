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
   @file extension.c
   @brief Plugable post-processing hooks.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "extension.h"
#include "log.h"
#include "dl-ext-backend.h"
#include "py-ext-backend.h"
#include "blacklist.h"
#include "utils.h"

/*! @cond INTERNAL */
typedef struct
{
	char *name;             /* callback name */
	uint32_t argc;          /* number of optional function arguments */
	CallbackArgType *types; /* optional function argument data types */
} ExtensionCallback;

typedef enum
{
	EXTENSION_MODULE_TYPE_UNDEFINED,
	EXTENSION_MODULE_TYPE_SHARED_LIB,
	EXTENSION_MODULE_TYPE_PYTHON
} ExtensionModuleType;

static ExtensionModuleType
_extension_map_file_extension(const char *filename)
{
	ExtensionModuleType mod_type = EXTENSION_MODULE_TYPE_UNDEFINED;

	if(filename)
	{
		size_t len = strlen(filename);

		if(len > 3)
		{
			if(!strcmp(filename + len - 3, ".so"))
			{
				mod_type = EXTENSION_MODULE_TYPE_SHARED_LIB;
			}
			#ifdef WITH_PYTHON
			else if(!strcmp(filename + len - 3, ".py"))
			{
				mod_type = EXTENSION_MODULE_TYPE_PYTHON;
			}
			#endif
		}
	}

	return mod_type;
}

typedef struct
{
	char *filename;                /* filename of the module */
	char *name;                    /* module name */
	char *version;                 /* module version */
	char *description;             /* a brief description */
	ExtensionModuleType type;      /* type id */
	ExtensionBackendClass backend; /* backend functions */
	void *handle;                  /* backend handle */
	AssocArray *callbacks;         /* associative array containing callback names and ExtensionCallback instances */
} ExtensionModule;
/*! @endcond */

static ExtensionCallback *
_extension_callback_new(const char *name, uint32_t argc)
{
	ExtensionCallback *cb = NULL;

	assert(name != NULL);

	if(name)
	{
		cb = utils_new(1, ExtensionCallback);
		cb->name = utils_strdup(name);
		cb->argc = argc;

		if(argc)
		{
			cb->types = utils_new(argc, CallbackArgType);
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

		#ifdef WITH_PYTHON
		case EXTENSION_MODULE_TYPE_PYTHON:
			py_extension_backend_get_class(cls);
			break;
		#endif

		default:
			success = false;
	}

	return success;
}

static ExtensionModule *
_extension_module_new(const char *filename, ExtensionModuleType type)
{
	ExtensionModule *module = NULL;

	assert(filename != NULL);

	if(filename && type != EXTENSION_MODULE_TYPE_UNDEFINED)
	{
		module = utils_new(1, ExtensionModule);

		if(_extension_module_set_backend_class(&module->backend, type))
		{
			module->filename = utils_strdup(filename);
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
		TRACEF("extension", "Cleaning up module `%s'.", module->filename);

		if(module->callbacks)
		{
			assoc_array_destroy(module->callbacks);
		}

		if(module->handle)
		{
			module->backend.unload(module->handle);
		}

		free(module->filename);
		free(module->name);
		free(module->version);
		free(module->description);
		free(module);
	}
}

static void
_extension_manager_function_discovered(RegistrationCtx *ctx, const char *name, uint32_t argc, ...)
{
	ExtensionCallback *cb = NULL;

	assert(ctx != NULL);
	assert(name != NULL);
	assert(argc < 128);

	TRACEF("extension", "Discovered function: name=%s, argc=%d", name, argc);

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
				WARNINGF("extension", "Unexpected data type: %#x", val);

				/* failure => free memory */
				_extension_callback_free(cb);
				success = false;
			}
		}

		va_end(ap);

		if(success)
		{
			assoc_array_set(((ExtensionModule *)ctx)->callbacks, utils_strdup(cb->name), cb, true);
		}
	}
}

static void
_extension_manager_extension_registered(RegistrationCtx *ctx, const char *name, const char *version, const char *description)
{
	ExtensionModule *module;

	assert(ctx != NULL);

	module = (ExtensionModule *)ctx;

	DEBUGF("extension", "Extension registered: name=%s, version=%s, description=%s", name, version, description);

	if(name)
	{
		module->name = utils_strdup(name);
	}

	if(version)
	{
		module->version = utils_strdup(version);
	}

	if(description)
	{
		module->description = utils_strdup(description);
	}
}

static bool
_extension_manager_import_module(ExtensionManager *manager, const char *filename, ExtensionModuleType type)
{
	ExtensionModule *module;
	bool success = false;

	assert(manager != NULL);
	assert(filename != NULL);

	module = _extension_module_new(filename, type);

	if(module)
	{
		/* load module */
		if((module->handle = module->backend.load(module->filename, _extension_manager_extension_registered, (void *)module)))
		{
			assoc_array_set(manager->modules, utils_strdup(module->filename), module, true);

			/* discover functions */
			module->backend.discover(module->handle, _extension_manager_function_discovered, (void *)module);

			success = true;
		}
	}

	if(!success)
	{
		_extension_module_free(module);
	}

	return success;
}

static ExtensionModule *
_extension_manager_find_callback(ExtensionManager *manager, const char *name, ExtensionCallback **cb)
{
	AssocArrayIter iter;

	assert(manager != NULL);
	assert(name != NULL);
	assert(cb != NULL);

	assoc_array_iter_init(manager->modules, &iter);

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

ExtensionManager *
extension_manager_new(void)
{
	ExtensionManager *manager;

	manager = utils_new(1, ExtensionManager);
	manager->modules = assoc_array_new(str_compare, free, (FreeFunc)_extension_module_free);

	return manager;
}

void
extension_manager_destroy(ExtensionManager *manager)
{
	if(manager)
	{
		if(manager->modules)
		{
			assoc_array_destroy(manager->modules);
		}

		free(manager);
	}
}

bool
extension_manager_load_directory(ExtensionManager *manager, const char *path, char **err)
{
	bool success = false;
	DIR *pdir;
	Blacklist *blacklist;
	char msg[PATH_MAX + 64];

	assert(manager != NULL);
	assert(path != NULL);

	DEBUGF("extension", "Loading extensions from directory: %s", path);

	blacklist = blacklist_new();
	blacklist_load_default(blacklist);

	/* try to open extension folder */
	if((pdir = opendir(path)))
	{
		/* search for extensions */
		struct dirent *entry;

		success = true;
		entry = readdir(pdir);

		while(success && entry)
		{
			TRACEF("extension", "Found file: %s", entry->d_name);

 			ExtensionModuleType mod_type = _extension_map_file_extension(entry->d_name);

			if(mod_type != EXTENSION_MODULE_TYPE_UNDEFINED)
			{
				char filename[PATH_MAX];

				if(utils_path_join(path, entry->d_name, filename, PATH_MAX))
				{
					if(blacklist_matches(blacklist, filename))
					{
						DEBUGF("extension", "File is blacklisted: %s", filename);
					}
					else
					{
						DEBUGF("extension", "Importing extension: %s", filename);

						if(!_extension_manager_import_module(manager, filename, mod_type))
						{
							int len = snprintf(msg, sizeof(msg), "Couldn't load extension \"%s\".", filename);

							if(len > 0 && (size_t)len < sizeof(msg))
							{
								*err = strdup(msg);
							}

							success = false;
						}
					}
				}
				else
				{
					if(err)
					{
						*err = strdup("Path to extension file exceeds maximum allowed path length.");
					}

					success = false;
				}
			}
			else
			{
				TRACE("extension", "Ignoring file: unsupported file extension.");
			}

			entry = readdir(pdir);
		}

		closedir(pdir);
	}
	else
	{
		if(err)
		{
			int len = snprintf(msg, sizeof(msg), "Warning: couldn't open directory \"%s\".", path);

			if(len > 0 && (size_t)len < sizeof(msg))
			{
				*err = strdup(msg);
			}
		}
	}

	blacklist_destroy(blacklist);

	TRACEF("extension", "Import completed with status %d.", success);

	return success;
}

int
extension_manager_load_default(ExtensionManager *manager)
{
	const char *home;
	char *err = NULL;
	int count = 0;

	assert(manager != NULL);

	DEBUG("extension", "Loading extensions from default directories.");

	home = getenv("HOME");

	if(home)
	{
		char path[PATH_MAX];

		if(utils_path_join(home, ".efind/extensions", path, PATH_MAX))
		{
			if(extension_manager_load_directory(manager, path, NULL))
			{
				++count;
			}
		}
	}
	else
	{
		DEBUG("extension", "Couldn't find home directory.");
	}

	if(extension_manager_load_directory(manager, "/etc/efind/extensions", &err))
	{
		++count;
	}
	else if(err)
	{
		fprintf(stderr, "%s\n", err);
		free(err);
		err = NULL;
	}

	return count;
}

ExtensionCallbackStatus
extension_manager_test_callback(ExtensionManager *manager, const char *name, uint32_t argc, CallbackArgType *types)
{
	ExtensionCallback *cb;
	ExtensionCallbackStatus result = EXTENSION_CALLBACK_STATUS_NOT_FOUND;

	assert(manager != NULL);
	assert(name != NULL);

	if(name)
	{
		if(_extension_manager_find_callback(manager, name, &cb))
		{
			if(cb->argc != argc)
			{
				result = EXTENSION_CALLBACK_STATUS_INVALID_SIGNATURE;
			}
			else
			{
				result = EXTENSION_CALLBACK_STATUS_OK;

				for(uint32_t i = 0; i < argc; ++i)
				{
					if(types[i] != cb->types[i])
					{
						result = EXTENSION_CALLBACK_STATUS_INVALID_SIGNATURE;
						break;
					}
				}
			}
		}
	}

	return result;
}

ExtensionCallbackStatus
extension_manager_invoke(ExtensionManager *manager, const char *name, const char *filename, uint32_t argc, void *argv[], int *result)
{
	ExtensionCallbackStatus status = EXTENSION_CALLBACK_STATUS_NOT_FOUND;
	ExtensionModule *module;
	ExtensionCallback *cb;

	assert(manager != NULL);
	assert(name != NULL);
	assert(filename != NULL);

	TRACEF("extension", "Invoking function `%s' with %d parameter(s).", name, argc);

	if((module = _extension_manager_find_callback(manager, name, &cb)))
	{
		if(cb->argc == argc)
		{
			if(module->backend.invoke(module->handle, name, filename, argc, argv, result) == 0)
			{
				status = EXTENSION_CALLBACK_STATUS_OK;
			}
		}
		else
		{
			TRACEF("extension", "Signature check of function `%s' failed.", name);
			status = EXTENSION_CALLBACK_STATUS_INVALID_SIGNATURE;
		}
	}
	else
	{
		TRACEF("extension", "Function `%s' not found.", name);
	}

	return status;
}

void
extension_manager_export(ExtensionManager *manager, FILE *out)
{
	AssocArrayIter iter;

	assert(manager != NULL);
	assert(out != NULL);

	assoc_array_iter_init(manager->modules, &iter);

	while(assoc_array_iter_next(&iter))
	{
		ExtensionModule *module= (ExtensionModule *)assoc_array_iter_get_value(&iter);

		assert(module != NULL);
		assert(module->filename != NULL);

		fputs(module->filename, out);
		fputc('\n', out);
			
		if(module->name && module->version)
		{
			fprintf(out, "\t%s, version %s\n\n", module->name, module->version);
		}

		if(module->description)
		{
			fprintf(out, "\t%s\n\n", module->description);
		}

		AssocArrayIter cb_iter;

		assoc_array_iter_init(module->callbacks, &cb_iter);

		while(assoc_array_iter_next(&cb_iter))
		{
			ExtensionCallback *cb = (ExtensionCallback *)assoc_array_iter_get_value(&cb_iter);
			fprintf(out, "\t%s(", cb->name);

			for(size_t i = 0; i < cb->argc; ++i)
			{
				const char *type = "integer";

				if(cb->types[i] == CALLBACK_ARG_TYPE_STRING)
				{
					type = "string";
				}

				fputs(type, out);

				if(i < cb->argc - 1)
				{
					fputs(", ", out);
				}
			}

			fputs(")\n", out);
		}

		fputc('\n', out);
	}
}

ExtensionCallbackArgs *
extension_callback_args_new(uint32_t argc)
{
	ExtensionCallbackArgs *args;

	args = utils_new(1, ExtensionCallbackArgs);
	args->argc = argc;
	
	if(args->argc > 0)
	{
		args->argv = utils_new(argc, void *);
		args->types = utils_new(argc, CallbackArgType);
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

	args->argv[offset] = utils_new(1, int32_t);
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

	if(string)
	{
		args->argv[offset] = utils_strdup(string);
	}

	args->types[offset] = CALLBACK_ARG_TYPE_STRING;
}

