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
#include <linux/limits.h>

#include "extension-interface.h"


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
	CallbackArgType *types;
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
	 *\param fn function called for each found callback function
	 *\param ctx registration context
	 *
	 * Discovers available functions.
	 */
	void (*discover)(void *handle, RegisterCallback fn, RegistrationCtx *ctx);

	/*!
	 *\param handle backend handle
	 *\param name name of the function to invoke
	 *\param filename name of the found file
	 *\param argc number of optional arguments
	 *\param argv optional arguments
	 *\param result location to store callback result
	 *\return 0 on success
	 *
	 * Invokes a function.
	 */
	int (*invoke)(void *handle, const char *name, const char *filename, uint32_t argc, void **argv, int *result);

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
	EXTENSION_CALLBACK_STATUS_NOT_FOUND,
	/*! Invalid signature. */
	EXTENSION_CALLBACK_STATUS_INVALID_SIGNATURE
} ExtensionCallbackStatus;

/*!
 *\struct ExtensionCallbackArgs
 *\brief Function argument vector.
 */
typedef struct
{
	/*! Argument vector. */
	void **argv;
	/*! Size of the vector. */
	uint32_t argc;
	/*! Argument data types. */
	CallbackArgType *types;
} ExtensionCallbackArgs;

/**
 *\param path path of the extension directory
 *\param err destination to store error messages
 *\return an ExtensionDir instance or NULL on failure
 *
 * Loads extension from the given directory.
 */
ExtensionDir *extension_dir_load(const char *path, char **err);

/**
 *\param err destination to store error messages
 *\return an ExtensionDir instance or NULL on failure
 *
 * Tries to load extensions from ~/.efind/extensions or
 * /etc/efind/extensions.
 */
ExtensionDir *extension_dir_default(char **err);

/**
 *\param dir an ExtensionDir instance
 *
 * Frees an ExtensionDir instance.
 */
void extension_dir_destroy(ExtensionDir *dir);

/**
 *\param dir an ExtensionDir instance
 *\param name name of the callback to test
 *\param argc number of function arguments
 *\param types argument data types
 *\return status code
 *
 * Tests if a callback with the specified signature does exist.
 */
ExtensionCallbackStatus extension_dir_test_callback(ExtensionDir *dir, const char *name, uint32_t argc, CallbackArgType *types);

/**
 *\param dir an ExtensionDir instance
 *\param name name of the callback to execute
 *\param filename name of the file to test
 *\param argc number of additional function arguments
 *\param argv additional function arguments
 *\param result destination to store the result of the callback
 *\return status code
 *
 * Executes a callback.
 */
ExtensionCallbackStatus extension_dir_invoke(ExtensionDir *dir, const char *name, const char *filename, uint32_t argc, void **argv, int *result);

/**
 *\param argc number of arguments
 *\return a new ExtensionCallbackArgs instance
 *
 * Creates a new ExtensionCallbackArgs instance.
 */
ExtensionCallbackArgs *extension_callback_args_new(uint32_t argc);

/**
 *\param args ExtensionCallbackArgs instance to free
 *\return a new ExtensionCallbackArgs instance
 *
 * Frees an ExtensionCallbackArgs instance.
 */
void extension_callback_args_free(ExtensionCallbackArgs *args);

/**
 *\param args ExtensionCallbackArgs instance
 *\param offset position of the argument to set
 *\param value value to set
 *
 * Copies an int32_t value to the specified position of the argument vector.
 */
void extension_callback_args_set_integer(ExtensionCallbackArgs *args, uint32_t offset, int32_t value);

/**
 *\param args ExtensionCallbackArgs instance
 *\param offset position of the argument to set
 *\param value value to set
 *
 * Copies a string to the specified position of the argument vector.
 */
void extension_callback_args_set_string(ExtensionCallbackArgs *args, uint32_t offset, const char *value);

#endif

