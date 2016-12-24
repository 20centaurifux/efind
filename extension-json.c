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
 * \file extension-json.c
 * \brief Load modules from JSON files.
 * \author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 * \version 0.1.0
 * \date 24. December 2016
 */

#ifndef __EXTENSION_JSON_H__
#define __EXTENSION_JSON_H__

#include <jansson.h>
#include <assert.h>

#include "extension.h"
#include "utils.h"

static ExtensionCallback *
_extension_callback_from_json(void *ptr, char **err)
{
	ExtensionCallback *callback = NULL;
	const char *cb_name;
	json_t *desc;

	assert(ptr != NULL);

	/* get callback name */
	cb_name = json_object_iter_key(ptr);
	desc = json_object_iter_value(ptr);

	if(desc && json_is_object(desc))
	{
		/* get callback signature */
		json_t *child = json_object_get(desc, "args");

		if(child && json_is_array(child))
		{
			size_t len = json_array_size(child);

			if(len < UINT32_MAX)
			{
				callback = extension_callback_new(cb_name, (uint32_t)len);

				if(callback)
				{
					bool success = true;

					for(uint32_t i = 0; i < len && success; ++i)
					{
						json_t *type_name = json_array_get(child, i);

						if(json_is_string(type_name))
						{
							callback->types[i] = extension_callback_arg_type_try_parse(json_string_value(type_name));

							if(callback->types[i] == EXTENSION_CALLBACK_ARG_TYPE_UNDEFINED)
							{
								utils_strdup_printf(err,
										    "Callback \"%s\" has argument with unknown data type \"%s\".",
										    callback->name,
										    json_string_value(type_name));
								success = false;
							}
						}
						else
						{
							utils_strdup_printf(err, "Callback argument data type must be a string value (callback \"%s\").", cb_name);
							success = false;
						}
					}

					if(!success)
					{
						extension_callback_free(callback);
						callback = NULL;
					}
				}
				else
				{
					utils_strdup_printf(err, "Couldn't create callback \"%s\".", cb_name);
				}
			}
			else
			{
				utils_strdup_printf(err, "Number of arguments is greater han UINT32_MAX, are you insane?");
			}
		}
		else
		{
			utils_strdup_printf(err, "Couldn't get function signature of callback \"%s\".", cb_name);
		}
	}
	else if(json_is_null(desc))
	{
		callback = extension_callback_new(cb_name, 0);
	}
	else
	{
		utils_strdup_printf(err, "Description of callback \"%s\" is invalid.", cb_name);
	}

	return callback;
}

static ExtensionModuleType
_extension_module_get_type_from_json(json_t *json)
{
	json_t *child;
	ExtensionModuleType type = EXTENSION_MODULE_TYPE_UNDEFINED;

	assert(json != NULL);
 
	child = json_object_get(json, "type");

	if(child && json_is_string(child))
	{
		type = extension_module_type_try_parse(json_string_value(child));
	}

	return type;
}

static bool
_extension_module_get_callbacks_from_json(ExtensionModule *module, const char *module_name, json_t *json, char **err)
{
	json_t *child;
	bool success = false;

	assert(module != NULL);
	assert(json != NULL);

	child = json_object_get(json, "export");

	if(child && json_is_object(child))
	{
		void *iter = json_object_iter(child);

		success = true;

		while(iter && success)
		{
			ExtensionCallback *callback = _extension_callback_from_json(iter, err);

			if(callback)
			{
				extension_module_set_callback(module, callback);
			}
			else
			{
				success = false;
			}

			iter = json_object_iter_next(child, iter);
		}
	}
	else
	{
		utils_strdup_printf(err, "Module \"%s\" has no exported callbacks.", module_name);
	}

	return success;
}

static ExtensionModule *
_extension_module_from_json(const char *path, void *ptr, char **err)
{
	ExtensionModule *module = NULL;
	const char *module_name;
	ExtensionModuleType type;
	char filename[PATH_MAX];

	assert(path != NULL);
	assert(ptr != NULL);

	/* build extension filename */
	module_name = json_object_iter_key(ptr);

	if(utils_path_join(path, module_name, filename, PATH_MAX))
	{
		/* test if file does exist */
		struct stat stbuf;

		if(!stat(filename, &stbuf) && (stbuf.st_mode & S_IFMT) == S_IFREG)
		{
			json_t *child = json_object_iter_value(ptr);

			/* get module type */
			if(child && json_is_object(child))
			{
				type = _extension_module_get_type_from_json(child);

				if(type != EXTENSION_MODULE_TYPE_UNDEFINED)
				{
					/* create ExtensionModule instance & get callbacks */
					module = extension_module_new(filename, type);

					if(!_extension_module_get_callbacks_from_json(module, module_name, child, err))
					{
						extension_module_free(module);
						module = NULL;
					}
				}
				else
				{
					utils_strdup_printf(err, "Module \"%s\" has no valid type description.", module_name);
				}
			}
			else
			{
				utils_strdup_printf(err, "Module \"%s\" has no valid description.", module_name);
			}
		}
		else
		{
			utils_strdup_printf(err, "\"%s\" is not a regular file.", filename);
		}
	}
	else
	{
		utils_strdup_printf(err, "Path to \"extensions.js\" exceeds maximum allowed path length.");
	}

	return module;
}

bool
extension_json_load_file(ExtensionDir *dir, const char *filename, char **err)
{
	json_t *json;
	json_error_t json_err;
	bool success = false;

	assert(dir != NULL);
	assert(filename != NULL);

	/* load JSON file */
	json = json_load_file(filename, 0, &json_err);

	if(json && json_is_object(json))
	{
		/* success => read modules from parsed object */
		void *iter = json_object_iter(json);
		
		success = true;

		while(success && iter)
		{
			ExtensionModule *module = _extension_module_from_json(dir->path, iter, err);

			if(module)
			{
				if(!extension_dir_register_module(dir, module))
				{
					utils_strdup_printf(err, "Couldn't register module \"%s\".", filename);
					success = false;
				}
			}
			else
			{
				if(err && !*err)
				{
					utils_strdup_printf(err, "Couldn't load module \"%s\" from \"%s\".", json_object_iter_key(iter), filename);
				}

				success = false;
			}

			iter = json_object_iter_next(json, iter);
		}
	}
	else
	{
		if(json_err.text)
		{
			utils_strdup_printf(err, "Couldn't load \"extensions.js\": %s", json_err.text);
		}
		else
		{
			utils_strdup_printf(err, "Couldn't load \"extensions.js\"");
		}
	}

	if(json)
	{
		json_decref(json);
	}

	return success;
}
#endif

