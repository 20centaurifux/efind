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
 * \file extension-interface.h
 * \brief Interface for extension modules.
 * \author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 * \version 0.1.0
 * \date 29. December 2016
 */

#ifndef __EXTENSION_INTERFACE_H__
#define __EXTENSION_INTERFACE_H__

#include <stdint.h>

/*! Extension registration context. */
typedef void * RegistrationCtx;

/**
   @param ctx registration context
   @param name name of the extension
   @param description brief description of the extension
   @param version extension version information

   Registers an extension in the plugin context.
 */
typedef void(*RegisterExtension)(RegistrationCtx *ctx, const char *name, const char *version, const char *description);

/**
   @param ctx registration context
   @param name name of the function to register
   @param argc number of optional function arguments
   @param ... data types of the optional arguments

   Registers a function in the plugin context.
 */
typedef void(*RegisterCallback)(RegistrationCtx *ctx, const char *name, uint32_t argc, ...);

/**
 *\enum CallbackArgType
 *\brief Allowed data types for additional callback arguments,
 */
typedef enum
{
	/*! Undefined data type. */
	CALLBACK_ARG_TYPE_UNDEFINED,
	/*! Integer. */
	CALLBACK_ARG_TYPE_INTEGER,
	/*! String. */
	CALLBACK_ARG_TYPE_STRING,
} CallbackArgType;
#endif

