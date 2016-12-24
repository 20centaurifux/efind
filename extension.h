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
 * \file extension.h
 * \brief Plugable post-processing hooks.
 * \author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 * \version 0.1.0
 * \date 22. December 2016
 */

#ifndef __EXTENSION_H__
#define __EXTENSION_H__

#include <datatypes.h>
#include <sys/stat.h>
#include <linux/limits.h>

/**
 *\enum ExtensionCallbackArgType
 *\brief Allowed data types for additional callback arguments,
 */
typedef enum
{
	/*! Undefined data type. */
	EXTENSION_CALLBACK_ARG_TYPE_UNDEFINED,
	/*! Integer. */
	EXTENSION_CALLBACK_ARG_TYPE_INTEGER,
	/*! String. */
	EXTENSION_CALLBACK_ARG_TYPE_STRING,
} ExtensionCallbackArgType;

/**
 *\struct ExtensionCallback
 *\brief A callback description. A callback has a name and a type signature.
 */
typedef struct
{
	/*! The callback name. */
	char *name;
	/*! Number of function arguments. */
	uint32_t argc;
	/*! Function argument data types. */
	ExtensionCallbackArgType *types;
} ExtensionCallback;

/**
 *\enum ExtensionModuleType
 *\brief Extension module type.
 */
typedef enum
{
	/*! Undefined. */
	EXTENSION_MODULE_TYPE_UNDEFINED,
	/*! The module is a shared library. */
	EXTENSION_MODULE_TYPE_SHARED_LIB
} ExtensionModuleType;

/**
 *\struct ExtensionBackendClass
 *\brief Functions an extension backend has to provide.
 */
typedef struct _ExtensionBackendClass
{
	/*!
	 *\param filename name of the extension module's file
	 *\return backend handle
	 *
	 * Loads an extension module.
	 */
	void *(*load)(const char *filename);

	/*!
	 *\param handle backend handle
	 *\param name name of the function to invoke
	 *\param filename name of the found file
	 *\param stbuf status information of the found file
	 *\param argc number of optional arguments
	 *\param argv optional arguments
	 *\param result location to store callback result
	 *\return 0 on success
	 *
	 * Invokes a function.
	 */
	int (*invoke)(void *handle, const char *name, const char *filename, struct stat *stbuf, uint32_t argc, void **argv, int *result);

	/*!
	 *\param handle backend handle
	 *
	 * Unloads an extension module. This function has to free all resources.
	 */
	void (*unload)(void *handle);
} ExtensionBackendClass;

/**
 *\struct ExtensionModule
 *\brief Extension module meta information & callbacks.
 */
typedef struct
{
	/*! Filename of the module. */
	char *filename;
	/*! Module type. */
	ExtensionModuleType type;
	/*! Backend functions. */
	ExtensionBackendClass backend;
	/*! Backend handle. */
	void *handle;
	/*! Associative array containing callback names and ExtensionCallback instances. */
	AssocArray *callbacks;
} ExtensionModule;

/**
 *\struct ExtensionDir
 *\brief A directory containing extensions.
 */
typedef struct
{
	/*! Filename of the directory. */
	char *path;
	/*! A tree holding extension modules. */
	AssocArray *modules;
} ExtensionDir;

/**
 *\enum ExtensionCallbackStatus
 *\brief Callback status codes.
 */
typedef enum
{
	/*! No failure occured. */
	EXTENSION_CALLBACK_STATUS_OK,
	/*! Callback not found. */
	EXTENSION_CALLBACK_NOT_FOUND,
	/*! Invalid signature. */
	EXTENSION_CALLBACK_STATUS_INVALID_SIGNATURE
} ExtensionCallbackStatus;

/**
 *\param string string to parse
 *\return the mapped ExtensionCallbackArgType
 *
 * Tries to map a string to an ExtensionCallbackArgType.
 */
ExtensionCallbackArgType extension_callback_arg_type_try_parse(const char *string);

/**
 *\param callback ExtensionCallback to free
 *
 * Frees an ExtensionCallback instance.
 */
void extension_callback_free(ExtensionCallback *callback);

/**
 *\param name name of the callback
 *\param argc number of function arguments
 *\return a new ExtensionCallback instance or NULL on failure
 *
 * Creates a new ExtensionCallback instance.
 */
ExtensionCallback *extension_callback_new(const char *name, uint32_t argc);

/**
 *\param string string to parse
 *\return the mapped ExtensionModuleType
 *
 * Tries to map a string to an ExtensionModuleType.
 */
ExtensionModuleType extension_module_type_try_parse(const char *string);

/**
 *\param module ExtensionModule instance to free
 *
 * Frees an ExtensionModule instance.
 */
void extension_module_free(ExtensionModule *module);

/**
 *\param filename filename of the extension module
 *\param type module type
 *\return a new ExtensionCallback instance or NULL on failure
 *
 * Creates a new ExtensionModule instance.
 */
ExtensionModule *extension_module_new(const char *filename, ExtensionModuleType type);

/**
 *\param module an ExtensionModule instance
 *\param callback callback to set
 *
 * Adds a callback to the ExtensionModule instance.
 */
void extension_module_set_callback(ExtensionModule *module, ExtensionCallback *callback);

/**
 *\param path path of the extension directory
 *\param err destination to store error message
 *\return an ExtensionDir instance or NULL on failure
 *
 * Reads the "extensions.js" file from the given directory & stores
 * the found modules in an associative structure.
 */
ExtensionDir *extension_dir_load(const char *path, char **err);

/**
 *\param dir an ExtensionDir instance
 *
 * Frees an ExtensionDir instance.
 */
void extension_dir_destroy(ExtensionDir *dir);

/**
 *\param dir an ExtensionDir instance
 *\param module module to register
 *\return true on success
 *
 * Registers a module.
 */
bool extension_dir_register_module(ExtensionDir *dir, ExtensionModule *module);

/**
 *\param dir an ExtensionDir instance
 *\param name name of the callback to test
 *\param argc number of function arguments
 *\param types argument data types
 *\return status code
 *
 * Tests if a callback with the specified signature does exist.
 */
ExtensionCallbackStatus extension_dir_test_callback(ExtensionDir *dir, const char *name, uint32_t argc, ExtensionCallbackArgType *types);

/**
 *\param dir an ExtensionDir instance
 *\param name name of the callback to execute
 *\param filename name of the file to test
 *\param stbuf status information of the file to test
 *\param argc number of additional function arguments
 *\param argv additional function arguments
 *\param result destination to store the result of the callback
 *\return status code
 *
 * Executes a callback.
 */
ExtensionCallbackStatus extension_dir_invoke(ExtensionDir *dir, const char *name, const char *filename, struct stat *stbuf, uint32_t argc, void **argv, int *result);

#endif

