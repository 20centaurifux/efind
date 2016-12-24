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

	if(!cb_name)
	{
		utils_strdup_printf(err, "Couldn't determine callback name.");
		goto error;
	}

	desc = json_object_iter_value(ptr);

	if(json_is_object(desc))
	{
		/* get callback signature */
		json_t *child = json_object_get(desc, "args");

		if(child && json_is_array(child))
		{
			size_t len = json_array_size(child);

			if(len > UINT32_MAX)
			{
				utils_strdup_printf(err, "Number of arguments is greater han UINT32_MAX, are you insane?");
				goto error;
			}

			callback = extension_callback_new(cb_name, (uint32_t)len);

			if(!callback)
			{
				utils_strdup_printf(err, "Couldn't create callback \"%s\"", cb_name);
				goto error;
			}

			for(uint32_t i = 0; i < len; ++i)
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
						goto error;
					}
				}
				else
				{
					utils_strdup_printf(err, "Callback argument data type must be a string value.");
					goto error;
				}
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
	extension_callback_free(callback);

	return NULL;
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

	if(!module_name)
	{
		utils_strdup_printf(err, "Found module without module name.");
		goto error;
	}

	if(!utils_path_join(path, module_name, filename, PATH_MAX))
	{
		utils_strdup_printf(err, "Path to \"extensions.js\" exceeds maximum allowed path length.");
		goto error;
	}

	/* test if file does exist */
	struct stat stbuf;

	if(stat(filename, &stbuf))
	{
		utils_strdup_printf(err, "Couldn't find \"%s\"", filename);
		goto error;
	}

	if((stbuf.st_mode & S_IFMT) != S_IFREG)
	{
		utils_strdup_printf(err, "\"%s\" is not a regular file.", filename);
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

	type = extension_module_type_try_parse(json_string_value(grandchild));

	if(type == EXTENSION_MODULE_TYPE_UNDEFINED)
	{
		utils_strdup_printf(err, "Module \"%s\" has an unknown type description.", module_name);
		goto error;
	}

	/* create module */
	module = extension_module_new(filename, type);

	if(!module)
	{
		utils_strdup_printf(err, "Initialization of module \"%s\" failed.", module_name);
		goto error;
	}

	/* add callbacks */
	grandchild = json_object_get(child, "export");

	if(!grandchild || !json_is_object(grandchild))
	{
		utils_strdup_printf(err, "Module \"%s\" has no exported callbacks.", module_name);
		goto error;
	}

	void *iter = json_object_iter(grandchild);

	while(iter)
	{
		ExtensionCallback *callback = _extension_callback_from_json(iter, err);

		if(!callback)
		{
			goto error;
		}

		extension_module_set_callback(module, callback);
		iter = json_object_iter_next(grandchild, iter);
	}

	return module;

error:
	extension_module_free(module);

	return NULL;
}

bool
extension_json_load_file(ExtensionDir *dir, const char *filename, char **err)
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

	void *iter = json_object_iter(json);

	while(iter)
	{
		ExtensionModule *module = _extension_module_from_json(dir->path, iter, err);

		if(!module)
		{
			goto out;
		}

		if(!extension_dir_register_module(dir, module))
		{
			utils_strdup_printf(err, "Couldn't register module \"%s\".", module->filename);
			goto out;
		}

		iter = json_object_iter_next(json, iter);
	}

	success = true;

out:
	json_decref(json);

	return success;
}
#endif

