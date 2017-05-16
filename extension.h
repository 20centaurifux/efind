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
   @file extension.h
   @brief Plugable post-processing hooks.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef EXTENSION_H
#define EXTENSION_H

#include <stdio.h>
#include <datatypes.h>
#include <limits.h>

#include "extension-interface.h"

/**
   @struct ExtensionBackendClass
   @brief Functions an extension backend has to provide.
 */
typedef struct _ExtensionBackendClass
{
	/**
	   @param filename name of the extension module file
	   @param fn function called to register the extension
	   @param ctx registration context
	   @return backend handle

	   Loads an extension module.
	 */
	void *(*load)(const char *filename, RegisterExtension fn, RegistrationCtx *ctx);

	/**
	   @param handle backend handle
	   @param fn function to register the callbacks of the extension
	   @param ctx registration context

	   Discovers available functions.
	 */
	void (*discover)(void *handle, RegisterCallback fn, RegistrationCtx *ctx);

	/**
	   @param handle backend handle
	   @param name name of the function to invoke
	   @param filename name of the found file
	   @param argc number of optional arguments
	   @param argv optional arguments
	   @param result location to store callback result
	   @return 0 on success

	   Invokes a function.
	 */
	int (*invoke)(void *handle, const char *name, const char *filename, uint32_t argc, void *argv[], int *result);

	/**
	   @param handle backend handle

	   Unloads an extension module. This function has to free all resources.
	 */
	void (*unload)(void *handle);
} ExtensionBackendClass;

/**
   @struct ExtensionManager
   @brief A list containing loaded extension files.
 */
typedef struct
{
	/*! Associative array containing module filenames and extension modules. */
	AssocArray *modules;
} ExtensionManager;

/**
   @enum ExtensionCallbackStatus
   @brief Callback status codes.
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

/**
   @struct ExtensionCallbackArgs
   @brief Function argument vector.
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
   @return new ExtensionManager

   Creates an ExtensionManager.
 */
ExtensionManager *extension_manager_new(void);

/**
   @param manager ExtensionManager to free

   Frees an ExtensionManager.
 */
void extension_manager_destroy(ExtensionManager *manager);

/**
   @param manager an ExtensionManager
   @param path path of the directory to load extensions from
   @param err destination to store error messages
   @return true on success

   Loads extensions from found in a directory.
 */
bool extension_manager_load_directory(ExtensionManager *manager, const char *path, char **err);

/**
   @param manager an ExtensionManager
   @return number of successfully read extension directories

   Tries to load extensions from ~/.efind/extensions and
   /etc/efind/extensions.
 */
int extension_manager_load_default(ExtensionManager *manager);

/**
   @param manager an ExtensionManager
   @param name name of the callback to test
   @param argc number of function arguments
   @param types argument data types
   @return status code

   Tests if a callback with the specified signature does exist.
 */
ExtensionCallbackStatus extension_manager_test_callback(ExtensionManager *manager, const char *name, uint32_t argc, CallbackArgType *types);

/**
   @param manager an ExtensionManager
   @param name name of the callback to invoke
   @param filename name of the file to test
   @param argc number of optional function arguments
   @param argv optional function arguments
   @param result destination to store the result of the callback
   @return status code

   Executes a callback.
 */
ExtensionCallbackStatus extension_manager_invoke(ExtensionManager *manager, const char *name, const char *filename, uint32_t argc, void *argv[], int *result);

/**
   @param manager an ExtensionManager
   @param out a stream

   Lists registered extensions and writes them to the specified stream.
 */
void extension_manager_export(ExtensionManager *manager, FILE *out);

/**
   @param argc number of arguments
   @return a new ExtensionCallbackArgs instance

   Creates a new ExtensionCallbackArgs instance.
 */
ExtensionCallbackArgs *extension_callback_args_new(uint32_t argc);

/**
   @param args ExtensionCallbackArgs instance to free

   Frees an ExtensionCallbackArgs instance.
 */
void extension_callback_args_free(ExtensionCallbackArgs *args);

/**
   @param args ExtensionCallbackArgs instance
   @param offset position of the argument to set
   @param value value to set

   Copies an int32_t value to the specified position of the argument vector.
 */
void extension_callback_args_set_integer(ExtensionCallbackArgs *args, uint32_t offset, int32_t value);

/**
   @param args ExtensionCallbackArgs instance
   @param offset position of the argument to set
   @param value value to set

   Copies a string to the specified position of the argument vector.
 */
void extension_callback_args_set_string(ExtensionCallbackArgs *args, uint32_t offset, const char *value);
#endif

