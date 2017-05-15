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
 * \file py-ext-backend.c
 * \brief Plugable post-processing hooks backend using libpython.
 * \author Sebastian Fedrau <sebastian.fedrau@gmail.com>
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
#include "utils.h"

typedef struct
{
	PyObject *module;
	AssocArray signatures;
} PyHandle;

static void
_py_set_python_path(void)
{
	PyObject *sys = PyImport_ImportModule("sys");

	if(sys && PyModule_Check(sys))
	{
		PyObject *path = PyObject_GetAttrString(sys, "path");

		if(path && PyList_Check(path))
		{
			PyList_Append(path, PyString_FromString("/etc/efind/extensions"));

			const char *homedir = getenv("HOME");

			if(homedir && *homedir)
			{
				size_t len = strlen(homedir) + 20;
				char *localpath = (char *)utils_malloc(sizeof(char) * len);

				sprintf(localpath, "%s/.efind/extensions", homedir);
				PyList_Append(path, PyString_FromString(localpath));
				free(localpath);
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
		name = basename(filename);

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

	for(i = 0; success && i < sizeof(keys) / sizeof(char *); i++)
	{
		success = false;

		PyObject *attr = PyObject_GetAttrString(module, keys[i]);

		if(attr && PyString_Check(attr))
		{
			details[i] = strdup(PyString_AsString(attr));
			success = true;
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
	PyObject *module = NULL;
	PyHandle *handle = NULL;

	assert(filename != NULL);

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
		if((module = PyImport_Import(name)))
		{
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
		}
		else
		{
			PyErr_Print();
		}

		Py_DECREF(name);
	}

	if(!handle)
	{
		/* Oops, something went wrong... */
		Py_XDECREF(module);
	}

	return handle;
}

static bool
_py_import_callable(PyHandle *handle, PyObject *callable, RegisterCallback fn, RegistrationCtx *ctx)
{
	bool success = false;

	assert(callable != NULL);
	assert(PyCallable_Check(callable));
	assert(fn != NULL);

	/* get function name */
	PyObject *name = PyObject_GetAttrString(callable, "__name__");

	if(name && PyString_Check(name))
	{
		/* get signature & register function */
		PyObject *sig = PyObject_GetAttrString(callable, "__signature__");

		if(sig && PySequence_Check(sig))
		{
			success = true;
			
			Py_ssize_t len = PySequence_Length(sig);

			if(len && len <= UINT32_MAX)
			{
				uint32_t argc = (uint32_t)len;
				int *signature = (int *)utils_malloc(sizeof(int) * len);

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
							success = false;
						}
					}
					else
					{
						success = false;
					}

					Py_DECREF(arg);
				}

				if(success)
				{
					const char *fn_name = PyString_AsString(name);

					/* call register function */
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
					assoc_array_set(&handle->signatures, strdup(fn_name), signature, false);
				}
				else
				{
					free(signature);
				}
			}
		}
		else 
		{
			success = (sig == Py_None);
		}

		Py_XDECREF(sig);
	}

	Py_XDECREF(name);

	return success;
}

static void
_py_ext_discover(void *handle, RegisterCallback fn, RegistrationCtx *ctx)
{
	assert(handle != NULL);
	assert(((PyHandle *)handle)->module != NULL);
	assert(PyModule_Check(((PyHandle *)handle)->module));
	assert(fn != NULL);

	PyObject *exports = PyObject_GetAttrString((PyObject *)((PyHandle *)handle)->module, "EXTENSION_EXPORT");

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

	Py_XDECREF(exports);
}

static int
_py_ext_backend_invoke(void *handle, const char *name, const char *filename, uint32_t argc, void **argv, int *result)
{
	int ret = -1;

	assert(handle != NULL);
	assert(((PyHandle *)handle)->module != NULL);
	assert(PyModule_Check(((PyHandle *)handle)->module));
	assert(name != NULL);
	assert(filename != NULL);

	/* find callable */
	PyObject *callable = PyObject_GetAttrString((PyObject *)((PyHandle *)handle)->module, name);

	if(callable && PyCallable_Check(callable))
	{
		/* get function signature */
		int *sig = assoc_array_lookup(&((PyHandle *)handle)->signatures, name);

		assert(sig != NULL);

		/* build argument list */
		PyObject *tuple = PyTuple_New(argc + 1);

		PyTuple_SetItem(tuple, 0, PyString_FromString(filename));

		for(int i = 0; i < argc; ++i)
		{
			PyObject *arg = NULL;

			if(sig[i] == CALLBACK_ARG_TYPE_INTEGER)
			{
				arg = PyInt_FromLong(*((int *)(argv[i])));
			}
			else if(sig[i] == CALLBACK_ARG_TYPE_STRING)
			{
				arg = PyString_FromString(argv[i]);
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
#endif

