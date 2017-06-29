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
   @file py-ext-backend.c
   @brief Plugable post-processing hooks backend using libpython2.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifdef WITH_PYTHON

#include <Python.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <avcall.h>
#include <datatypes.h>
#include <assert.h>

#include "py-ext-backend.h"
#include "extension-interface.h"
#include "log.h"
#include "utils.h"

typedef struct
{
	PyObject *module;
	AssocArray signatures;
} PyHandle;

/* add extension paths to sys.path */
static void
_py_set_python_path(void)
{
	PyObject *sys = PyImport_ImportModule("sys");

	TRACE("python", "Importing `sys' module.");

	if(sys && PyModule_Check(sys))
	{
		TRACE("python", "Retrieving `path' attribute from `sys' module.");

		PyObject *path = PyObject_GetAttrString(sys, "path");

		if(path && PyList_Check(path))
		{
			DEBUG("python", "Appending global extension directory to Python path: /etc/efind/extension");

			PyList_Append(path, PyString_FromString("/etc/efind/extensions"));

			const char *homedir = getenv("HOME");

			if(homedir && *homedir)
			{
				size_t len = strlen(homedir);

				if(SIZE_MAX - 20 >= len)
				{
					char *localpath = (char *)utils_malloc(sizeof(char) * (len + 20));

					sprintf(localpath, "%s/.efind/extensions", homedir);

					DEBUGF("python", "Appending local extension directory to Python path: %s", localpath);

					PyList_Append(path, PyString_FromString(localpath));
					free(localpath);
				}
			}
		}

		Py_XDECREF(path);
	}

	Py_XDECREF(sys);
}

static void
_py_initialize(void)
{
	if(!Py_IsInitialized())
	{
		Py_Initialize();
		_py_set_python_path();
		atexit(Py_Finalize);
	}
}

static void
_py_handle_destroy(PyHandle *handle)
{
	if(handle)
	{
		assert(!handle->module || PyModule_Check(handle->module));

		Py_XDECREF(handle->module);
		assoc_array_free(&handle->signatures);
		free(handle);
	}
}

static PyHandle *
_py_handle_new(PyObject *module)
{
	PyHandle *handle = NULL;

	assert(module != NULL);
	assert(PyModule_Check(module));

	handle = (PyHandle *)utils_malloc(sizeof(PyHandle));

	handle->module = module;
	assoc_array_init(&handle->signatures, str_compare, free, free);

	return handle;
}

static char *
_py_get_module_name(const char *filename)
{
	char *name = NULL;

	assert(filename != NULL);

	if(filename)
	{
		name = utils_strdup(basename(filename));

		if(name)
		{
			size_t len = strlen(name);

			if(len > 3)
			{
				name[len - 3] = '\0';
			}
			else
			{
				free(name);
				name = NULL;
			}
		}
	}

	return name;
}

static void
_py_free_extension_details(char **details, int count)
{
	assert(details != NULL);
	assert(count >= 0 && count <= 3);

	for(int i = 0; i < count; i++)
	{
		free(details[i]);
	}
}

bool
_py_get_extension_details(PyObject *module, char *details[3])
{
	bool success = true;
	static char *keys[3] = {"EXTENSION_NAME", "EXTENSION_VERSION", "EXTENSION_DESCRIPTION"};
	size_t i;

	assert(module != NULL);
	assert(details != NULL);

	for(i = 0; success && i < sizeof(keys) / sizeof(char *); ++i)
	{
		success = false;

		TRACEF("python", "Searching for symbol: `%s'", keys[i]);

		PyObject *attr = PyObject_GetAttrString(module, keys[i]);

		if(attr && PyString_Check(attr))
		{
			details[i] = utils_strdup(PyString_AsString(attr));
			success = true;
		}
		else
		{
			DEBUGF("python", "Couldn't find string `%s'.", keys[i]);
		}

		Py_XDECREF(attr);
	}

	if(!success)
	{
		_py_free_extension_details(details, i - 1);
	}

	return success;
}

static void *
_py_ext_backend_load(const char *filename, RegisterExtension fn, RegistrationCtx *ctx)
{
	char *mod_name;
	PyHandle *handle = NULL;

	assert(filename != NULL);
	assert(fn != NULL);

	/* get module name from filename */
	mod_name = _py_get_module_name(filename);

	if(!mod_name)
	{
		return NULL;
	}

	/* initialize Python interpreter (if necessary) */
	_py_initialize();

	/* try to import specified module */
	PyObject *name = PyString_FromString(mod_name);

	if(name)
	{
		PyObject *module = NULL;

		DEBUGF("python", "Importing `%s' module.", mod_name);

		if((module = PyImport_Import(name)))
		{
			TRACEF("python", "Module `%s' imported successfully, retrieving details.", mod_name);

			/* module imported successfully => get details */
			char *details[3];

			if(_py_get_extension_details(module, details))
			{
				/* register loaded module */
				fn(ctx, details[0], details[1], details[2]);
				_py_free_extension_details(details, 3);

				/* create handle */
				handle = _py_handle_new(module);
			}
			else
			{
				/* Ooops, something went wrong */
				DEBUGF("python", "Couldn't retrieve required details from module `%s'.", mod_name);
				Py_DECREF(module);
			}
		}
		else
		{
			PyErr_Print();
		}

		Py_DECREF(name);
	}

	free(mod_name);

	return handle;
}

static bool
_py_import_callable(PyHandle *handle, PyObject *callable, RegisterCallback fn, RegistrationCtx *ctx)
{
	bool success = false;

	assert(handle != NULL);
	assert(callable != NULL);
	assert(PyCallable_Check(callable));
	assert(fn != NULL);

	/* get function name */
	PyObject *name = PyObject_GetAttrString(callable, "__name__");

	if(name && PyString_Check(name))
	{
		const char *fn_name = PyString_AsString(name);

		assert(fn_name != NULL);

		TRACEF("python", "Importing function: `%s'", fn_name);

		/* get signature & register function */
		PyObject *sig = NULL;
		uint32_t argc = 0;
		int *signature = NULL;

		if(PyObject_HasAttrString(callable, "__signature__"))
		{
			sig = PyObject_GetAttrString(callable, "__signature__");
		}

		if(sig && PySequence_Check(sig))
		{
			success = true;
			
			Py_ssize_t len = PySequence_Length(sig);

			if(len && len <= UINT32_MAX)
			{
				argc = (uint32_t)len;
				signature = (int *)utils_malloc(sizeof(int) * len);

				for(Py_ssize_t i = 0; success && i < PySequence_Length(sig); ++i)
				{
					PyObject *arg = PySequence_ITEM(sig, i);

					if(PyType_Check(arg))
					{
						if((PyTypeObject *)arg == &PyInt_Type)
						{
							signature[i] = CALLBACK_ARG_TYPE_INTEGER;
						}
						else if((PyTypeObject *)arg == &PyString_Type)
						{
							signature[i] = CALLBACK_ARG_TYPE_STRING;
						}
						else
						{
							DEBUGF("python", "__signature__ attribute of function `%s' contains an unsupported type.", fn_name);
							success = false;
						}
					}
					else
					{
						DEBUGF("python", "__signature__ attribute of function `%s' is invalid, PyType_Check() failed.", fn_name);
						success = false;
					}

					Py_DECREF(arg);
				}
			}
		}
		else
		{

			success = !sig || sig == Py_None;

			if(!success)
			{
				DEBUGF("python", "__signature__ attribute of function `%s' is invalid.", fn_name);
			}
		}

		Py_XDECREF(sig);

		/* call registration function */
		if(success)
		{
			av_alist alist;

			av_start_void(alist, fn);
			av_ptr(alist, RegistrationCtx*, ctx);
			av_ptr(alist, const char*, fn_name);
			av_int(alist, argc);

			for(uint32_t i = 0; i < argc; ++i)
			{
				av_int(alist, signature[i]);
			}

			av_call(alist);

			/* store signature */
			assoc_array_set(&handle->signatures, utils_strdup(fn_name), signature, false);
		}
		else
		{
			free(signature);
		}
	}

	Py_XDECREF(name);

	return success;
}

static void
_py_ext_discover(void *handle, RegisterCallback fn, RegistrationCtx *ctx)
{
	PyHandle *py_handle = (PyHandle *)handle;

	assert(py_handle != NULL);
	assert(py_handle->module != NULL);
	assert(PyModule_Check(py_handle->module));
	assert(fn != NULL);

	TRACE("python", "Searching for symbol: `EXTENSION_EXPORT'");

	PyObject *exports = PyObject_GetAttrString((PyObject *)py_handle->module, "EXTENSION_EXPORT");

	if(exports && PySequence_Check(exports))
	{
		bool success = true;

		for(Py_ssize_t i = 0; success && i < PySequence_Length(exports); ++i)
		{
			success = false;
			PyObject *callable = PySequence_ITEM(exports, i);

			if(PyCallable_Check(callable))
			{
				success = _py_import_callable(handle, callable, fn, ctx);
			}

			Py_DECREF(callable);
		}
	}
	else
	{
		DEBUG("python", "Couldn't find sequence `EXTENSION_EXPORT'.");
	}

	Py_XDECREF(exports);
}

static int
_py_ext_backend_invoke(void *handle, const char *name, const char *filename, uint32_t argc, void **argv, int *result)
{
	PyHandle *py_handle = (PyHandle *)handle;
	int ret = -1;

	assert(py_handle != NULL);
	assert(py_handle->module != NULL);
	assert(PyModule_Check(py_handle->module));
	assert(name != NULL);
	assert(filename != NULL);

	/* find callable */
	PyObject *callable = PyObject_GetAttrString(py_handle->module, name);

	if(callable && PyCallable_Check(callable))
	{
		/* get function signature */
		int *sig = assoc_array_lookup(&py_handle->signatures, name);

		/* build argument list */
		PyObject *tuple = PyTuple_New(argc + 1);

		PyTuple_SetItem(tuple, 0, PyString_FromString(filename));

		for(uint32_t i = 0; i < argc; ++i)
		{
			PyObject *arg = NULL;

			if(sig[i] == CALLBACK_ARG_TYPE_INTEGER)
			{
				arg = PyInt_FromLong(*((int *)(argv[i])));
			}
			else if(sig[i] == CALLBACK_ARG_TYPE_STRING)
			{
				if(argv[i])
				{
					arg = PyString_FromString(argv[i]);
				}
				else
				{
					arg = PyString_FromString("");
				}
			}

			PyTuple_SetItem(tuple, i + 1, arg);
		}

		PyObject *obj = PyObject_CallObject(callable, tuple);

		if(obj)
		{
			if(PyInt_Check(obj))
			{
				long value = PyInt_AsLong(obj);

				if(value >= INT_MIN && value <= INT_MAX)
				{
					*result = (int)value;
					ret = 0;
				}
			}

			Py_DECREF(obj);
		}
		else
		{
			PyErr_Print();
		}

		Py_DECREF(tuple);
	}
	else
	{
		ERRORF("python", "Callbable `%s' not found.", name);
	}

	Py_XDECREF(callable);

	return ret;
}

static void
_py_ext_backend_unload(void *handle)
{
	assert(handle != NULL);

	_py_handle_destroy(handle);
}

void
py_extension_backend_get_class(ExtensionBackendClass *cls)
{
	assert(cls != NULL);

	cls->load = _py_ext_backend_load;
	cls->discover = _py_ext_discover;
	cls->invoke = _py_ext_backend_invoke;
	cls->unload = _py_ext_backend_unload;
}
#endif // WITH_PYTHON

