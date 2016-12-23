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

typedef struct _ExtensionBackendClass
{
	void *(*load)(const char *filename);
	int (*invoke)(void *handle, const char *name, const char *filename, struct stat *stbuf, uint32_t argc, void **argv, int *result);
	void (*unload)(void *handle);
} ExtensionBackendClass;

/**
 *\struct ExtensionDir
 *\brief A directory containing extensions.
 */
typedef struct
{
	/*! Filename of the directory. */
	char path[PATH_MAX];
	/*! A tree holding extension modules. */
	AssocArray *modules;
} ExtensionDir;

/**
 *\enum Allowed data types for additional callback arguments,
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
 *\enum ExtensionExecutionStatus
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

