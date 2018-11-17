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
#include <ffi.h>
#include <datatypes.h>
#include <assert.h>

#include "py-ext-backend.h"
#include "extension-interface.h"
#include "log.h"
#include "utils.h"
#include "pathbuilder.h"

typedef struct
{
	PyObject *module;
	AssocArray signatures;
} PyHandle;

static void
_py_append_global_extension_path(PyObject *path)
{
	assert(path != NULL);
	assert(PyList_Check(path));

	char dir[PATH_MAX];

	if(path_builder_global_extensions(dir, PATH_MAX))
	{
		DEBUGF("python", "Appending global extension directory to Python path: %s", dir);

		PyList_Append(path, PyUnicode_FromString(dir));
	}
	else
	{
		WARNING("python", "Couldn't build global extension path.");
	}
}
	
static void
_py_append_local_extension_path(PyObject *path)
{
	assert(path != NULL);
	assert(PyList_Check(path));

	char localpath[PATH_MAX];

	if(path_builder_local_extensions(localpath, PATH_MAX))
	{
		DEBUGF("python", "Appending local extension directory to Python path: %s", localpath);

		PyList_Append(path, PyUnicode_FromString(localpath));
	}
	else
	{
		WARNING("python", "Couldn't build local extension path.");
	}
}

static void
_py_append_extension_paths_from_env(PyObject *path)
{
	assert(path != NULL);
	assert(PyList_Check(path));

	char *dirs = getenv("EFIND_EXTENSION_PATH");

	if(dirs && *dirs)
	{
		dirs = utils_strdup(dirs); // prevent strtok_r from changing environment variable

		char *rest = dirs;
		char *dir;

		while((dir = strtok_r(rest, ":", &rest)))
		{
			DEBUGF("python", "Appending extension directory to Python path: %s", dir);

			PyList_Append(path, PyUnicode_FromString(dir));
		}

		free(dirs);
	}
}

static void
_py_append_extension_module_paths(void)
{
	TRACE("python", "Importing `sys' module.");

	PyObject *sys = PyImport_ImportModule("sys");

	if(sys)
	{
		assert(PyModule_Check(sys));

		TRACE("python", "Retrieving `path' attribute from `sys' module.");

		PyObject *path = PyObject_GetAttrString(sys, "path");

		if(path)
		{
			assert(PyList_Check(path));

			TRACE("python", "Appending extension directories.");

			_py_append_global_extension_path(path);
			_py_append_local_extension_path(path);
			_py_append_extension_paths_from_env(path);

			Py_DECREF(path);
		}
		else
		{
			PyErr_Print();
		}

		Py_DECREF(sys);
	}
	else
	{
		PyErr_Print();
	}
}

static void
_py_initialize(void)
{
	if(!Py_IsInitialized())
	{
		Py_Initialize();
		_py_append_extension_module_paths();
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
	assert(module != NULL);
	assert(PyModule_Check(module));

	PyHandle *handle = utils_new(1, PyHandle);

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
	size_t count;

	assert(module != NULL);
	assert(details != NULL);

	for(count = 0; success && count < sizeof(keys) / sizeof(char *); count++)
	{
		TRACEF("python", "Searching for symbol: `%s'", keys[count]);

		PyObject *attr = PyObject_GetAttrString(module, keys[count]);

		if(attr)
		{
			success = PyUnicode_Check(attr);

			if(success)
			{
				details[count] = utils_strdup(PyUnicode_AsUTF8(attr));
			}
			else
			{
				DEBUGF("python", "Couldn't find string `%s'.", keys[count]);
			}

			Py_DECREF(attr);
		}
		else
		{
			PyErr_Print();
			success = false;
		}
	}

	if(!success)
	{
		_py_free_extension_details(details, count - 1);
	}

	return success;
}

static PyHandle *
_py_import_module(const char *name, RegisterExtension fn, RegistrationCtx *ctx)
{
	PyHandle *handle = NULL;

	assert(name != NULL);
	assert(fn != NULL);

	PyObject *module = PyImport_ImportModule(name);

	if(module)
	{
		TRACEF("python", "Module `%s' imported successfully, retrieving details.", name);

		char *details[3];

		if(_py_get_extension_details(module, details))
		{
			fn(ctx, details[0], details[1], details[2]);
			_py_free_extension_details(details, 3);
			handle = _py_handle_new(module);
		}
		else
		{
			DEBUGF("python", "Couldn't retrieve required details from module `%s'.", name);

			Py_DECREF(module);
		}
	}
	else
	{
		PyErr_Print();
	}

	return handle;
}

static void *
_py_ext_backend_load(const char *filename, RegisterExtension fn, RegistrationCtx *ctx)
{
	PyHandle *handle = NULL;

	assert(filename != NULL);
	assert(fn != NULL);

	char *name = _py_get_module_name(filename);

	if(name)
	{
		_py_initialize();

		DEBUGF("python", "Importing `%s' module.", name);

		handle = _py_import_module(name, fn, ctx);

		free(name);
	}
	else
	{
		DEBUGF("python", "Couldn't get module name from filename: `%s'", filename);
	}

	return handle;
}

static bool
_py_build_signature_from_sequence(PyObject *seq, uint32_t *argc, int **signature)
{
	bool success = true;

	assert(seq != NULL);
	assert(PySequence_Check(seq));
	assert(argc != NULL);
	assert(*argc == 0);
	assert(signature != NULL);
	assert(*signature == NULL);

	Py_ssize_t len = PySequence_Length(seq);

	if(len > UINT32_MAX)
	{
		DEBUG("python", "Signature exceeds allowed maximum length.");
	}
	else if(len)
	{
		*argc = (uint32_t)len;
		*signature = utils_new(len, int);

		for(Py_ssize_t i = 0; success && i < len; i++)
		{
			PyObject *arg = PySequence_ITEM(seq, i);

			if(PyType_Check(arg))
			{
				if((PyTypeObject *)arg == &PyLong_Type)
				{
					(*signature)[i] = CALLBACK_ARG_TYPE_INTEGER;
				}
				else if((PyTypeObject *)arg == &PyUnicode_Type)
				{
					(*signature)[i] = CALLBACK_ARG_TYPE_STRING;
				}
				else
				{
					DEBUG("python", "__signature__ attribute contains an unsupported type.");
					success = false;
				}
			}
			else
			{
				DEBUG("python", "__signature__ attribute is invalid, PyType_Check() failed.");
				success = false;
			}

			Py_XDECREF(arg);
		}
	}

	return success;
}

static bool
_py_get_signature_from_callable(PyObject *callable, uint32_t *argc, int **signature)
{
	PyObject *sig = NULL;
	bool success = false;

	assert(callable != NULL);
	assert(PyCallable_Check(callable));
	assert(argc != NULL);
	assert(*argc == 0);
	assert(signature != NULL);
	assert(*signature == NULL);

	if(PyObject_HasAttrString(callable, "__signature__"))
	{
		sig = PyObject_GetAttrString(callable, "__signature__");
	}

	if(sig && PySequence_Check(sig))
	{
		success = _py_build_signature_from_sequence(sig, argc, signature);
	}
	else
	{
		success = !sig || sig == Py_None;

		if(!success)
		{
			DEBUG("python", "__signature__ attribute is invalid.");
		}
	}

	Py_XDECREF(sig);

	return success;
}

static bool
_py_register_callable(PyObject *callable, char *fn_name, uint32_t argc, int *signature, RegisterCallback fn, RegistrationCtx *ctx)
{
	ffi_cif cif;
	ffi_type *ret_type = &ffi_type_void;
	ffi_arg ret_value;
	ffi_type *arg_types[3 + argc];
	void *arg_values[3 + argc];
	bool success = false;

	arg_types[0] = &ffi_type_pointer;
	arg_types[1] = &ffi_type_pointer;
	arg_types[2] = &ffi_type_uint;

	arg_values[0] = &ctx;
	arg_values[1] = &fn_name;
	arg_values[2] = &argc;

	for(uint32_t i = 0; i < argc; i++)
	{
		arg_types[3 + i] = &ffi_type_sint;
		arg_values[3 + i] = &signature[i];
	}

	ffi_status status = ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, 3, 3 + argc, ret_type, arg_types);

	if(status == FFI_OK)
	{
		ffi_call(&cif, FFI_FN(fn), &ret_value, arg_values);
		success = true;
	}
	else
	{
		WARNINGF("python", "`ffi_prep_cif_var' failed with error code %#x.", status);
	}

	return success;
}

static bool
_py_import_callable(PyHandle *handle, PyObject *callable, RegisterCallback fn, RegistrationCtx *ctx)
{
	bool success = false;

	assert(handle != NULL);
	assert(callable != NULL);
	assert(PyCallable_Check(callable));
	assert(fn != NULL);

	PyObject *name = PyObject_GetAttrString(callable, "__name__");

	if(name)
	{
		if(PyUnicode_Check(name))
		{
			char *fn_name = PyUnicode_AsUTF8(name);

			assert(fn_name != NULL);

			TRACEF("python", "Importing function: `%s'", fn_name);

			uint32_t argc = 0;
			int *signature = NULL;

			if(_py_get_signature_from_callable(callable, &argc, &signature))
			{
				if(_py_register_callable(callable, fn_name, argc, signature, fn, ctx))
				{
					assoc_array_set(&handle->signatures, utils_strdup(fn_name), signature, false);
					success = true;
				}
			}

			if(!success && signature)
			{
				free(signature);
			}
		}
		else
		{
			ERROR("python", "Couldn't detect name of callable object.");
		}

		Py_DECREF(name);
	}
	else
	{
		PyErr_Print();
	}

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

	if(exports)
	{
		if(PySequence_Check(exports))
		{
			Py_ssize_t len = PySequence_Length(exports);
			bool success = true;

			for(Py_ssize_t i = 0; success && i < len; i++)
			{
				success = false;
				PyObject *callable = PySequence_ITEM(exports, i);

				if(PyCallable_Check(callable))
				{
					success = _py_import_callable(handle, callable, fn, ctx);
				}
				else
				{
					DEBUGF("python", "Object at position %l of sequence `EXTENSION_EXPORT' is not a callable.", i);
				}

				Py_DECREF(callable);
			}
		}
		else
		{
			DEBUG("python", "Couldn't find sequence `EXTENSION_EXPORT'.");
		}

		Py_DECREF(exports);
	}
	else
	{
		PyErr_Print();
	}
}

static PyObject *
_py_build_function_tuple(const char *filename, uint32_t argc, void **argv, int *sig)
{
	PyObject *tuple = NULL;

	assert(filename != NULL);
	assert(!argc || sig != NULL);

	tuple = PyTuple_New(argc + 1);

	PyTuple_SetItem(tuple, 0, PyUnicode_FromString(filename));

	for(uint32_t i = 0; i < argc; i++)
	{
		PyObject *arg = NULL;

		if(sig[i] == CALLBACK_ARG_TYPE_INTEGER)
		{
			arg = PyLong_FromLong(*((int *)(argv[i])));
		}
		else if(sig[i] == CALLBACK_ARG_TYPE_STRING)
		{
			if(argv[i])
			{
				arg = PyUnicode_FromString(argv[i]);
			}
			else
			{
				arg = PyUnicode_FromString("");
			}
		}
		else
		{
			ERRORF("python", "Unknown datatype in function signature: %#x\n", sig[i]);
		}

		PyTuple_SetItem(tuple, i + 1, arg);
	}

	return tuple;
}

static int
_py_invoke(PyObject *callable, PyObject *tuple, int *result)
{
	int ret = -1;

	assert(callable != NULL);
	assert(PyCallable_Check(callable));
	assert(tuple != NULL);
	assert(PyTuple_Check(tuple));
	assert(result != NULL);

	*result = 0;

	PyObject *obj = PyObject_CallObject(callable, tuple);

	if(obj)
	{
		if(PyLong_Check(obj))
		{
			long value = PyLong_AsLong(obj);

			if(value >= INT_MIN && value <= INT_MAX)
			{
				*result = (int)value;
				ret = 0;
			}
			else
			{
				DEBUG("python", "Result out of range.");
			}
		}
		else
		{
			DEBUG("python", "Result is not an integer.");
		}

		Py_DECREF(obj);
	}
	else
	{
		PyErr_Print();
	}

	return ret;
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

	PyObject *callable = PyObject_GetAttrString(py_handle->module, name);

	if(callable)
	{
		if(PyCallable_Check(callable))
		{
			AssocArrayPair *pair = assoc_array_lookup(&py_handle->signatures, name);
			int *sig = assoc_array_pair_get_value(pair);

			PyObject *tuple = _py_build_function_tuple(filename, argc, argv, sig);

			assert(tuple != NULL);

			ret = _py_invoke(callable, tuple, result);

			Py_DECREF(tuple);
		}
		else
		{
			ERRORF("python", "Callable `%s' not found.", name);
		}

		Py_DECREF(callable);
	}
	else
	{
		PyErr_Print();
	}

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

